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
static uint8_t  cascade_step;          // 0..4 — current bar being revealed
static uint16_t cascade_start_frame;   // global_frame_count snapshot at init
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

    // Index into PUZZLES for the puzzle just completed. current_puzzle_index
    // was incremented by PLAY's win-transition code, so the just-completed
    // puzzle is at index-1. Defensive: cap at 0 if somehow underflowed.
    uint8_t prev_idx = (win_save.current_puzzle_index == 0)
        ? 0 : (uint8_t)(win_save.current_puzzle_index - 1);
    const Puzzle *puzzle = &PUZZLES[prev_idx];

    // Reveal bars 0..cascade_step-1 (tiers 0=yellow .. 3=purple)
    for (uint8_t i = 0; i < 4 && i < cascade_step; i++) {
        render_bar(puzzle, i, (uint8_t)(3 + i));
    }

    if (cascade_step >= 4) {
        char buf[21];

        // Per-puzzle stats (Plan C): TRIES, TIME, ATTEMPT from
        // last_puzzle_result populated by PLAY at submission time.
        sprintf(buf, "PUZZLE %d", (int)win_save.current_puzzle_index);
        render_text(2, 9, buf);

        sprintf(buf, "TRIES: %d/4", (int)last_puzzle_result.tries_used);
        render_text(2, 10, buf);

        uint16_t mins = (uint16_t)(last_puzzle_result.elapsed_seconds / 60);
        uint16_t secs = (uint16_t)(last_puzzle_result.elapsed_seconds % 60);
        sprintf(buf, "TIME:  %d:%02d", (int)mins, (int)secs);
        render_text(2, 11, buf);

        sprintf(buf, "ATTEMPT: %d", (int)last_puzzle_result.attempt_number);
        render_text(2, 12, buf);

        // Lifetime stats
        sprintf(buf, "STREAK:%d  BEST:%d",
                (int)win_save.current_streak,
                (int)win_save.best_streak);
        render_text(2, 14, buf);

        render_text(2, 16, "START  NEXT");
        render_text(2, 17, "SELECT TITLE");
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
