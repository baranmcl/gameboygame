#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
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
    redraw_needed = true;
    sfx_lose();
    // Same shape as BAR_CASCADE — 4 bars × 20 frames each = 80 frames.
    // Engine just counts frames; scene_lose advances reveal_step.
    anim_start(ANIM_LOSE_REVEAL, 0, 80);
}

static void lose_render(void) {
    if (!redraw_needed) return;
    redraw_needed = false;

    render_clear();
    render_text(3, 1, "OUT OF TRIES");

    // Reveal bars 0..reveal_step-1 (tiers 0=yellow .. 3=purple) at full width.
    const Puzzle *puzzle = &PUZZLES[lose_save.current_puzzle_index];
    for (uint8_t i = 0; i < 4 && i < reveal_step; i++) {
        render_bar(puzzle, i, (uint8_t)(3 + i), SCREEN_TILES_W);
    }
    // v1.2 Phase 2: the currently-sliding bar (if cascade still in progress).
    if (reveal_step < 4 && slide_cols > 0) {
        render_bar(puzzle, reveal_step, (uint8_t)(3 + reveal_step), slide_cols);
    }

    if (reveal_step >= 4) {
        char buf[21];
        sprintf(buf, "ATTEMPT: %d", (int)lose_save.current_puzzle_fails);
        render_text(2, 9, buf);

        // Menu (RETRY always; SKIP only if fails >= 3). Use ternary on
        // full string literals to sidestep the SDCC %c sprintf bug.
        render_text(2, 12, (lose_menu_cursor == 0) ? ">RETRY PUZZLE" : " RETRY PUZZLE");
        if (lose_skip_available) {
            render_text(2, 13, (lose_menu_cursor == 1) ? ">SKIP PUZZLE" : " SKIP PUZZLE");
        }

        render_text(2, 16, "SELECT TITLE");
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
            // RETRY — no save change needed (ip_* already cleared on PLAY→LOSE)
            sfx_select();
            *next_scene = SCENE_PLAY;
        } else {
            // SKIP — advance puzzle, reset streak, increment skip counter
            lose_save.current_puzzle_index++;
            lose_save.current_streak = 0;
            lose_save.puzzles_skipped_total++;
            lose_save.current_puzzle_fails = 0;
            save_store(&lose_save);
            sfx_skip();
            *next_scene = SCENE_PLAY;
        }
    } else if (input_pressed(BTN_SELECT)) {
        sfx_deselect();
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
