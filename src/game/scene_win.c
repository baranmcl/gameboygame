#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "scene_handoff.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include "../engine/anim.h"
#include <gb/gb.h>
#include <stdio.h>
#include <stdbool.h>

static GameSave win_save;
static uint8_t  cascade_step;                // 0..4 — current bar being revealed
static uint16_t cascade_start_frame;         // global_frame_count snapshot at init
static bool     stats_fade_triggered;        // fire STATS_FADE once after cascade ends
static uint8_t  stats_step;                  // 0..6 — typewriter progress (lines revealed)
static uint16_t stats_start_frame;           // global_frame_count when stats reveal started
static uint8_t  last_rendered_cascade_step;  // diff against cascade_step in win_render
static uint8_t  last_rendered_stats_step;    // diff against stats_step in win_render
static uint8_t  slide_cols;                  // v1.2: 0..20, columns of the currently-sliding bar painted
static uint8_t  last_rendered_slide_cols;    // diff against slide_cols for the in-progress bar
// v1.2 Phase 3: tiered milestones. Set in win_init from the loaded save.
// Rendered AFTER the stats typewriter completes (replaces the standard
// menu hints at rows 16-17 when active). Streak wins priority over
// lifetime when both hit on the same WIN.
//
// all_puzzles_done is a third class that layers ON TOP of the streak/
// lifetime celebration — when the player completes the last uncompleted
// puzzle in the bank, "ALL PUZZLES DONE!" appears on row 15 above the
// other milestone (or stands alone on row 16 if no streak/lifetime hit).
static bool     milestone_lifetime_hit;
static bool     milestone_streak_hit;
static bool     all_puzzles_done;             // count(completed_bits) == NUM_PUZZLES
static bool     milestone_displayed;          // one-shot: text painted
static bool     milestone_fanfare_played;     // one-shot: SFX fired
static bool     redraw_needed;

extern volatile uint16_t global_frame_count;

// v1.2 Phase 2: reveal_cols parameter for slide-in animation. When
// reveal_cols == SCREEN_TILES_W (20), paints the full bar (matches v1.1
// behavior). When < 20, paints only the leftmost `reveal_cols` columns;
// the right side stays as whatever was previously there (typically
// blank, since we never re-render bars after they're fully shown).
// Category name + palette writes are gated on full reveal to avoid
// chopped-letter artifacts during slide-in.
static void render_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y, uint8_t reveal_cols) {
    uint8_t pattern_tile = (uint8_t)(UI_TILE_PATTERN_BASE + tier);
    for (uint8_t x = 0; x < 3; x++) {
        if (x < reveal_cols) render_set_tile(x, y, pattern_tile);
    }
    for (uint8_t x = 17; x < 20; x++) {
        if (x < reveal_cols) render_set_tile(x, y, pattern_tile);
    }
    for (uint8_t x = 3; x < 17; x++) {
        if (x < reveal_cols) render_set_tile(x, y, UI_TILE_SOLID_DARK);
    }

    // Overlay category name + tier palette only on full reveal — avoids
    // partial letters / partial palette tinting during slide-in.
    if (reveal_cols >= SCREEN_TILES_W) {
        const char *name = puzzle->category_names[tier];
        uint8_t name_len = 0;
        while (name[name_len] && name_len < 14) name_len++;
        uint8_t text_x = (uint8_t)(3 + (14 - name_len) / 2);
        render_text_inv(text_x, y, name);

        // Plan D Phase 3: tier-color palette over the whole 20-tile row.
        uint8_t palette = (uint8_t)(GBC_PAL_TIER_YELLOW + tier);
        for (uint8_t x = 0; x < SCREEN_TILES_W; x++) {
            render_set_tile_palette(x, y, palette);
        }
    }
}

static void win_init(void) {
    BGP_REG = 0xE4;  // defensive: see comment in scene_play's play_init
    save_load(&win_save);
    cascade_step = 0;
    cascade_start_frame = global_frame_count;
    stats_fade_triggered = false;
    stats_step = 0;
    stats_start_frame = 0;
    last_rendered_cascade_step = 0;
    last_rendered_stats_step = 0;
    slide_cols = 0;
    last_rendered_slide_cols = 0;

    // v1.2 Phase 3: milestone detection. scene_play already incremented
    // puzzles_solved_total and current_streak before transitioning here,
    // so the loaded values are the post-win totals.
    milestone_lifetime_hit = (win_save.puzzles_solved_total > 0
                           && (win_save.puzzles_solved_total % 5) == 0);
    milestone_streak_hit   = (win_save.current_streak > 0
                           && (win_save.current_streak % 5) == 0);

    // all_puzzles_done: count bits in completed_bits and compare to
    // NUM_PUZZLES. Bit count caps at NUM_PUZZLES (each puzzle has one
    // bit), so this cleanly answers "every puzzle done?" regardless
    // of how the player got there (linear, replay, mixed). Doesn't
    // use puzzles_solved_total because cycle-restart can push that
    // past NUM_PUZZLES.
    {
        uint8_t bit_count = 0;
        for (uint8_t i = 0; i < (MAX_PUZZLES_SUPPORTED / 8); i++) {
            uint8_t byte = win_save.completed_bits[i];
            while (byte) {
                bit_count += (uint8_t)(byte & 1);
                byte >>= 1;
            }
        }
        all_puzzles_done = (bit_count >= NUM_PUZZLES);
    }

    milestone_displayed      = false;
    milestone_fanfare_played = false;

    redraw_needed = false;
    sfx_win();

    // One-shot clear + "SOLVED!" header here, so win_render can be purely
    // additive (no render_clear in the hot path). Plan B's redraw pattern
    // called render_clear + full re-render on every state change; for the
    // typewriter sequence (6 stats steps over ~500ms) that overshoots
    // VBlank and causes visible flicker. New approach: only render the
    // CHANGES since the last frame (new bar appearing, new stats line).
    render_clear();
    render_text(6, 1, "SOLVED!");

    // BAR_CASCADE = 4 bars × 20 frames = 80 frames total.
    // Engine just counts frames; scene_win advances cascade_step + gates input.
    anim_start(ANIM_BAR_CASCADE, 0, 80);
}

static void win_render(void) {
    if (!redraw_needed) return;
    redraw_needed = false;

    // Index into PUZZLES for the puzzle just completed (index was
    // incremented by PLAY's win-transition; defensive cap at 0).
    uint8_t prev_idx = (win_save.current_puzzle_index == 0)
        ? 0 : (uint8_t)(win_save.current_puzzle_index - 1);
    const Puzzle *puzzle = &PUZZLES[prev_idx];

    // Render any NEWLY-fully-revealed bars (one or more per redraw call).
    // Re-rendering an already-shown bar is wasted work but harmless;
    // tracking last_rendered_cascade_step minimizes it to one bar per step.
    // Each fully-revealed bar gets reveal_cols = SCREEN_TILES_W (full).
    while (last_rendered_cascade_step < cascade_step
        && last_rendered_cascade_step < 4) {
        render_bar(puzzle,
                   last_rendered_cascade_step,
                   (uint8_t)(3 + last_rendered_cascade_step),
                   SCREEN_TILES_W);
        last_rendered_cascade_step++;
        last_rendered_slide_cols = 0;  // next bar's slide-in starts from zero
    }

    // v1.2 Phase 2: the currently-sliding bar (if any) needs incremental
    // redraw as slide_cols grows. cascade_step < 4 means there's a bar
    // still revealing; render it with the current slide_cols width.
    // Only re-render when slide_cols changed to avoid wasted tile writes.
    if (cascade_step < 4 && last_rendered_slide_cols != slide_cols) {
        render_bar(puzzle, cascade_step,
                   (uint8_t)(3 + cascade_step),
                   slide_cols);
        last_rendered_slide_cols = slide_cols;
    }

    // Render NEWLY-revealed stats lines (one per typewriter step).
    char buf[21];
    while (last_rendered_stats_step < stats_step) {
        last_rendered_stats_step++;
        switch (last_rendered_stats_step) {
            case 1:
                sprintf(buf, "PUZZLE %d", (int)win_save.current_puzzle_index);
                render_text(2, 9, buf);
                break;
            case 2:
                sprintf(buf, "TRIES: %d/4", (int)last_puzzle_result.tries_used);
                render_text(2, 10, buf);
                break;
            case 3: {
                uint16_t mins = (uint16_t)(last_puzzle_result.elapsed_seconds / 60);
                uint16_t secs = (uint16_t)(last_puzzle_result.elapsed_seconds % 60);
                sprintf(buf, "TIME:  %d:%02d", (int)mins, (int)secs);
                render_text(2, 11, buf);
                break;
            }
            case 4:
                sprintf(buf, "ATTEMPT: %d", (int)last_puzzle_result.attempt_number);
                render_text(2, 12, buf);
                break;
            case 5:
                sprintf(buf, "STREAK:%d  BEST:%d",
                        (int)win_save.current_streak,
                        (int)win_save.best_streak);
                render_text(2, 14, buf);
                break;
            case 6:
                // v1.2 Phase 3: when ANY milestone is active (lifetime,
                // streak, or all_puzzles_done), the milestone text below
                // replaces the standard menu hints. When none active,
                // render the standard hints here (v1.1 behavior).
                if (!milestone_streak_hit && !milestone_lifetime_hit
                                          && !all_puzzles_done) {
                    render_text(2, 16, "START  NEXT");
                    render_text(2, 17, "SELECT TITLE");
                }
                break;
        }
    }

    // v1.2 Phase 3: milestone text overlay. Three classes that stack:
    //
    //   Row 15: "ALL PUZZLES DONE!" if all_puzzles_done (highest tier;
    //           only fires when every puzzle has been completed)
    //   Row 16: "STREAK N!" (streak priority over lifetime if both hit)
    //           OR "ALL PUZZLES DONE!" (if all_puzzles_done is the ONLY
    //           milestone — moves to row 16 instead of row 15)
    //   Row 17: "PRESS START!" if any milestone active
    //
    // Painted ONCE after the typewriter completes, guarded by
    // milestone_displayed.
    if (last_rendered_stats_step >= 6 && !milestone_displayed) {
        char buf[21];
        bool combined_milestone = milestone_streak_hit || milestone_lifetime_hit;

        if (all_puzzles_done && combined_milestone) {
            // Both: ALL PUZZLES DONE on row 15, streak/lifetime on row 16
            render_text(1, 15, "ALL PUZZLES DONE!");
            if (milestone_streak_hit) {
                sprintf(buf, "STREAK %d!", (int)win_save.current_streak);
                render_text(5, 16, buf);
            } else {
                sprintf(buf, "%d SOLVED!", (int)win_save.puzzles_solved_total);
                render_text(5, 16, buf);
            }
            render_text(2, 17, "PRESS START!");
        } else if (all_puzzles_done) {
            // Only all-done — promote it to row 16 alone.
            render_text(1, 16, "ALL PUZZLES DONE!");
            render_text(2, 17, "PRESS START!");
        } else if (milestone_streak_hit) {
            sprintf(buf, "STREAK %d!", (int)win_save.current_streak);
            render_text(5, 16, buf);
            render_text(2, 17, "PRESS START!");
        } else if (milestone_lifetime_hit) {
            sprintf(buf, "%d SOLVED!", (int)win_save.puzzles_solved_total);
            render_text(5, 16, buf);
            render_text(2, 17, "PRESS START!");
        }
        milestone_displayed = true;
    }
}

static void win_update(Scene *next_scene) {
    // Derive cascade_step from global_frame_count, not main-loop iteration
    // counts (which throttle behind 60Hz when redraw_needed triggers
    // expensive render passes — see scene_lose.c for full rationale).
    if (anim_current() == ANIM_BAR_CASCADE) {
        uint16_t elapsed = (uint16_t)(global_frame_count - cascade_start_frame);
        uint8_t expected_step = (uint8_t)(elapsed / 20);
        if (expected_step > 4) expected_step = 4;
        if (expected_step != cascade_step) {
            cascade_step = expected_step;
            redraw_needed = true;
        }

        // v1.2 Phase 2: drive slide_cols for the currently-revealing bar.
        // Each bar's 20-frame slot subdivides into 5 frames of slide-in
        // (4 cols/frame: 4, 8, 12, 16, 20) + 15 frames fully revealed.
        // Only compute when there's still a bar in progress (cascade_step
        // < 4); after the last bar fully reveals we let slide stay at 20.
        if (cascade_step < 4) {
            uint16_t bar_local = (uint16_t)(elapsed % 20);
            uint8_t new_slide;
            if (bar_local < 5) {
                new_slide = (uint8_t)((bar_local + 1) * 4);
            } else {
                new_slide = SCREEN_TILES_W;  // 20
            }
            if (new_slide != slide_cols) {
                slide_cols = new_slide;
                redraw_needed = true;
            }
        }
        return;
    }

    // Plan C: trigger STATS_FADE timer once when BAR_CASCADE ends.
    // Animation engine just counts the 30-frame duration; the visual
    // (per-line reveal) is driven by stats_step computed from elapsed
    // frames below. Spec said palette swap, but global BGP changes
    // dimmed the already-visible "SOLVED!" header + bar labels too —
    // user preferred the typewriter alternative (visual stays bounded
    // to the stats region).
    if (cascade_step >= 4 && !stats_fade_triggered) {
        stats_fade_triggered = true;
        stats_start_frame = global_frame_count;
        anim_start(ANIM_STATS_FADE, 0, 30);
    }

    // Advance the typewriter step while STATS_FADE is running. 30 frames
    // total / 5 frames per step = 6 steps revealed (one per line).
    if (anim_current() == ANIM_STATS_FADE) {
        uint16_t elapsed = (uint16_t)(global_frame_count - stats_start_frame);
        uint8_t expected = (uint8_t)(elapsed / 5);
        if (expected > 6) expected = 6;
        if (expected != stats_step) {
            stats_step = expected;
            redraw_needed = true;
        }
    } else if (stats_fade_triggered && stats_step < 6) {
        // Anim ended but we hadn't reached step 6 — snap to fully revealed.
        stats_step = 6;
        redraw_needed = true;
    }

    if (anim_is_playing()) return;

    // v1.2 Phase 3: milestone fanfare. Fires ONCE on the first frame
    // after stats_step reaches 6 AND any milestone is hit. Reuses
    // existing SFX (sfx_win for the bigger achievements — streak OR
    // all-puzzles-done; sfx_correct for the gentler lifetime chime)
    // to keep sound vocabulary consistent.
    if (stats_step >= 6 && !milestone_fanfare_played) {
        if (milestone_streak_hit || all_puzzles_done) {
            sfx_win();
        } else if (milestone_lifetime_hit) {
            sfx_correct();
        }
        milestone_fanfare_played = true;
    }

    if (input_pressed(BTN_START)) {
        sfx_select();
        // If we just finished the last puzzle, transition to ALL_DONE
        // instead of PLAY. NUM_PUZZLES from puzzles_types.h.
        if (win_save.current_puzzle_index >= NUM_PUZZLES) {
            *next_scene = SCENE_ALL_DONE;
        } else {
            *next_scene = SCENE_PLAY;
        }
    } else if (input_pressed(BTN_SELECT)) {
        sfx_deselect();
        *next_scene = SCENE_TITLE;
    }
}

static void win_teardown(void) {
}

const SceneVTable SCENE_WIN_VTABLE = {
    .init = win_init,
    .update = win_update,
    .render = win_render,
    .teardown = win_teardown,
};
