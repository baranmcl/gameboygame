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
static bool     redraw_needed;

extern volatile uint16_t global_frame_count;

static void render_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y) {
    // Same shape as scene_play's solved bar (Plan C now overlays the
    // category name via render_text_inv).
    uint8_t pattern_tile = (uint8_t)(UI_TILE_PATTERN_BASE + tier);
    for (uint8_t x = 0; x < 3; x++)   render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 17; x < 20; x++) render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 3; x < 17; x++)  render_set_tile(x, y, UI_TILE_SOLID_DARK);

    const char *name = puzzle->category_names[tier];
    uint8_t name_len = 0;
    while (name[name_len] && name_len < 14) name_len++;
    uint8_t text_x = (uint8_t)(3 + (14 - name_len) / 2);
    render_text_inv(text_x, y, name);
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

    // Render any NEWLY-revealed bars (one or more per redraw call).
    // Re-rendering an already-shown bar is wasted work but harmless;
    // tracking last_rendered_cascade_step minimizes it to one bar per step.
    while (last_rendered_cascade_step < cascade_step
        && last_rendered_cascade_step < 4) {
        render_bar(puzzle,
                   last_rendered_cascade_step,
                   (uint8_t)(3 + last_rendered_cascade_step));
        last_rendered_cascade_step++;
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
                render_text(2, 16, "START  NEXT");
                render_text(2, 17, "SELECT TITLE");
                break;
        }
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
