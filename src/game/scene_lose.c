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

static GameSave lose_save;
static uint8_t  lose_menu_cursor;
static bool     lose_skip_available;   // true if current_puzzle_fails >= 3
static uint8_t  reveal_step;            // 0..4 — current bar being revealed
static uint16_t reveal_start_frame;     // global_frame_count snapshot at init
static uint8_t  slide_cols;             // v1.2: 0..20, columns of the currently-sliding bar painted
// v1.2 Phase 2 (refactor): incremental-render trackers. Same anti-flicker
// pattern as scene_win — render_clear is one-shot in lose_init; lose_render
// only paints DELTAS so the tilemap buffer stays in a valid state at every
// frame (no VBlank race that would expose a half-cleared buffer to the
// LCD). Without these, the slide_cols animation (5x as many redraws as
// the v1.1 bar reveal) triggered visible per-frame screen flashes.
static uint8_t  last_rendered_reveal_step;
static uint8_t  last_rendered_slide_cols;
static bool     menu_rendered;            // one-shot: paint static menu chrome (ATTEMPT, SELECT TITLE) once when reveal completes
static uint8_t  last_rendered_menu_cursor; // diff against lose_menu_cursor for the > marker
static bool     redraw_needed;

extern volatile uint16_t global_frame_count;

// v1.2 Phase 2: reveal_cols parameter for slide-in animation (matches
// scene_win's render_bar). Full reveal = SCREEN_TILES_W; partial reveal
// paints only the leftmost columns + skips name + skips palette.
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

    if (reveal_cols >= SCREEN_TILES_W) {
        const char *name = puzzle->category_names[tier];
        uint8_t name_len = 0;
        while (name[name_len] && name_len < 14) name_len++;
        uint8_t text_x = (uint8_t)(3 + (14 - name_len) / 2);
        render_text_inv(text_x, y, name);

        uint8_t palette = (uint8_t)(GBC_PAL_TIER_YELLOW + tier);
        for (uint8_t x = 0; x < SCREEN_TILES_W; x++) {
            render_set_tile_palette(x, y, palette);
        }
    }
}

static void lose_init(void) {
    BGP_REG = 0xE4;  // defensive: see comment in scene_play's play_init
    save_load(&lose_save);
    lose_menu_cursor = 0;
    lose_skip_available = (lose_save.current_puzzle_fails >= 3);
    reveal_step = 0;
    reveal_start_frame = global_frame_count;
    slide_cols = 0;
    last_rendered_reveal_step = 0;
    last_rendered_slide_cols = 0;
    menu_rendered = false;
    last_rendered_menu_cursor = 0xFF;  // sentinel — forces first menu paint
    redraw_needed = false;             // lose_render is now purely additive
    sfx_lose();

    // v1.2 Phase 2 (refactor): clear + paint the persistent header HERE.
    // lose_render below is purely additive (no render_clear in the hot
    // path) — needed to avoid VBlank racing the slide animation's
    // ~20 redraws.
    render_clear();
    render_text(3, 1, "OUT OF TRIES");

    // Same shape as BAR_CASCADE — 4 bars × 20 frames each = 80 frames.
    // Engine just counts frames; scene_lose advances reveal_step.
    anim_start(ANIM_LOSE_REVEAL, 0, 80);
}

static void lose_render(void) {
    if (!redraw_needed) return;
    redraw_needed = false;

    const Puzzle *puzzle = &PUZZLES[lose_save.current_puzzle_index];

    // Paint any NEWLY-fully-revealed bars (one per redraw step typically).
    while (last_rendered_reveal_step < reveal_step
        && last_rendered_reveal_step < 4) {
        render_bar(puzzle, last_rendered_reveal_step,
                   (uint8_t)(3 + last_rendered_reveal_step),
                   SCREEN_TILES_W);
        last_rendered_reveal_step++;
        last_rendered_slide_cols = 0;  // next bar slides in fresh
    }

    // The currently-sliding bar (if cascade still in progress).
    if (reveal_step < 4 && last_rendered_slide_cols != slide_cols) {
        render_bar(puzzle, reveal_step,
                   (uint8_t)(3 + reveal_step),
                   slide_cols);
        last_rendered_slide_cols = slide_cols;
    }

    // Once all 4 bars revealed: paint the static menu chrome ONCE
    // (ATTEMPT line + SELECT TITLE hint), then handle menu cursor
    // updates incrementally below.
    if (reveal_step >= 4 && !menu_rendered) {
        char buf[21];
        sprintf(buf, "ATTEMPT: %d", (int)lose_save.current_puzzle_fails);
        render_text(2, 9, buf);
        render_text(2, 16, "SELECT TITLE");
        menu_rendered = true;
    }

    // Menu cursor lines re-paint whenever the cursor moves OR when the
    // menu first appears. We always paint both RETRY and SKIP rows when
    // dirty — overwriting the existing 13-char line in place (same length
    // every time, no clearing needed). SKIP only painted if available.
    if (reveal_step >= 4 && last_rendered_menu_cursor != lose_menu_cursor) {
        render_text(2, 12, (lose_menu_cursor == 0) ? ">RETRY PUZZLE" : " RETRY PUZZLE");
        if (lose_skip_available) {
            render_text(2, 13, (lose_menu_cursor == 1) ? ">SKIP PUZZLE" : " SKIP PUZZLE");
        }
        last_rendered_menu_cursor = lose_menu_cursor;
    }
}

static void lose_update(Scene *next_scene) {
    // Derive reveal_step from frames elapsed since lose_init, not from
    // main-loop iteration counts. Main loop can lag behind the 60Hz
    // VBlank rate when redraw_needed triggers expensive render passes;
    // global_frame_count is incremented in the VBlank ISR so it stays
    // accurate regardless of main loop throughput.
    if (anim_current() == ANIM_LOSE_REVEAL) {
        uint16_t elapsed = (uint16_t)(global_frame_count - reveal_start_frame);
        uint8_t expected_step = (uint8_t)(elapsed / 20);
        if (expected_step > 4) expected_step = 4;
        if (expected_step != reveal_step) {
            reveal_step = expected_step;
            slide_cols = 0;  // next bar's slide-in starts fresh
            redraw_needed = true;
        }

        // v1.2 Phase 2: drive slide_cols for the currently-revealing bar.
        // Same 5-frames-of-slide-in + 15-frames-fully-revealed pattern as
        // scene_win. Only animate if there's still a bar in progress.
        if (reveal_step < 4) {
            uint16_t bar_local = (uint16_t)(elapsed % 20);
            uint8_t new_slide;
            if (bar_local < 5) {
                new_slide = (uint8_t)((bar_local + 1) * 4);
            } else {
                new_slide = SCREEN_TILES_W;
            }
            if (new_slide != slide_cols) {
                slide_cols = new_slide;
                redraw_needed = true;
            }
        }
        return;
    }
    if (anim_is_playing()) return;

    uint8_t menu_count = lose_skip_available ? 2 : 1;

    if (input_repeat(BTN_UP) && menu_count > 1) {
        lose_menu_cursor = (uint8_t)((lose_menu_cursor + menu_count - 1) % menu_count);
        redraw_needed = true;
        sfx_move();
    }
    if (input_repeat(BTN_DOWN) && menu_count > 1) {
        lose_menu_cursor = (uint8_t)((lose_menu_cursor + 1) % menu_count);
        redraw_needed = true;
        sfx_move();
    }

    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
        if (lose_menu_cursor == 0) {
            // RETRY — restart same puzzle. In replay mode, stays in replay
            // (replay_puzzle_index unchanged) so scene_play re-loads the
            // same replayed puzzle. In linear, just re-enters PLAY normally.
            // No save change needed either way (ip_* already cleared).
            sfx_select();
            *next_scene = SCENE_PLAY;
        } else {
            // SKIP — only meaningful in linear mode. In replay, SKIP
            // is a discretionary "give up on this replay" → return to
            // puzzle-select without mutating any linear state.
            if (replay_puzzle_index != REPLAY_NONE) {
                sfx_skip();
                replay_puzzle_index = REPLAY_NONE;
                *next_scene = SCENE_PUZZLE_SELECT;
            } else {
                // Linear SKIP — advance puzzle, reset streak, increment counter.
                lose_save.current_puzzle_index++;
                lose_save.current_streak = 0;
                lose_save.puzzles_skipped_total++;
                lose_save.current_puzzle_fails = 0;
                save_store(&lose_save);
                sfx_skip();
                *next_scene = SCENE_PLAY;
            }
        }
    } else if (input_pressed(BTN_SELECT)) {
        sfx_deselect();
        // v1.2 Phase 7: SELECT-quit always returns to TITLE; clear
        // replay flag so the next session starts in linear mode.
        replay_puzzle_index = REPLAY_NONE;
        *next_scene = SCENE_TITLE;
    }
}

static void lose_teardown(void) {
}

const SceneVTable SCENE_LOSE_VTABLE = {
    .init = lose_init,
    .update = lose_update,
    .render = lose_render,
    .teardown = lose_teardown,
};
