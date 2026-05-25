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

static GameSave win_save;
static uint8_t  cascade_step;          // 0..4 — current bar being revealed
static uint16_t cascade_start_frame;   // global_frame_count snapshot at init
static bool     redraw_needed;

extern volatile uint16_t global_frame_count;

static void render_bar(uint8_t tier, uint8_t y) {
    // Same shape as scene_play's solved bar (no category-name overlay
    // since font is dark-on-light; Plan C adds inverted font).
    uint8_t pattern_tile = (uint8_t)(UI_TILE_PATTERN_BASE + tier);
    for (uint8_t x = 0; x < 3; x++)   render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 17; x < 20; x++) render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 3; x < 17; x++)  render_set_tile(x, y, UI_TILE_SOLID_DARK);
}

static void win_init(void) {
    BGP_REG = 0xE4;  // defensive: see comment in scene_play's play_init
    save_load(&win_save);
    cascade_step = 0;
    cascade_start_frame = global_frame_count;
    redraw_needed = true;
    sfx_win();
    // BAR_CASCADE = 4 bars × 20 frames = 80 frames total.
    // Engine just counts frames; scene_win advances cascade_step + gates input.
    anim_start(ANIM_BAR_CASCADE, 0, 80);
}

static void win_render(void) {
    if (!redraw_needed) return;
    redraw_needed = false;

    render_clear();
    render_text(6, 1, "SOLVED!");

    // Reveal bars 0..cascade_step-1 (tiers 0=yellow .. 3=purple)
    for (uint8_t i = 0; i < 4 && i < cascade_step; i++) {
        render_bar(i, (uint8_t)(3 + i));
    }

    if (cascade_step >= 4) {
        char buf[21];
        // win_save.current_puzzle_index was incremented by Phase 6's
        // submission code before transitioning here, so it equals
        // (1-based) the puzzle the player just finished.
        sprintf(buf, "PUZZLE %d DONE", (int)win_save.current_puzzle_index);
        render_text(2, 9, buf);
        sprintf(buf, "STREAK: %d", (int)win_save.current_streak);
        render_text(2, 11, buf);
        sprintf(buf, "BEST:   %d", (int)win_save.best_streak);
        render_text(2, 12, buf);

        render_text(2, 15, "START  NEXT");
        render_text(2, 16, "SELECT TITLE");
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
