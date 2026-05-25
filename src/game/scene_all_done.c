#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include <gb/gb.h>
#include <stdio.h>
#include <stdbool.h>

static GameSave done_save;
static bool     redraw_needed;

static void done_init(void) {
    BGP_REG = 0xE4;  // defensive: see comment in scene_play's play_init
    save_load(&done_save);
    redraw_needed = true;
    sfx_win();  // celebrate (same fanfare as a normal WIN)
}

static void done_render(void) {
    if (!redraw_needed) return;
    redraw_needed = false;

    render_clear();

    char buf[21];
    sprintf(buf, "YOU SOLVED ALL %d!", (int)NUM_PUZZLES);
    render_text(1, 1, buf);

    sprintf(buf, "PUZZLES SOLVED: %d", (int)done_save.puzzles_solved_total);
    render_text(1, 5, buf);
    sprintf(buf, "SKIPPED: %d", (int)done_save.puzzles_skipped_total);
    render_text(1, 6, buf);
    sprintf(buf, "BEST STREAK: %d", (int)done_save.best_streak);
    render_text(1, 7, buf);

    // Average tries — integer math (no float on GBDK). Show one decimal
    // place via x10 scaling: avg = total_tries * 10 / solves; display
    // as N.M where N = avg/10, M = avg%10.
    if (done_save.puzzles_solved_total > 0) {
        uint16_t avg_x10 = (uint16_t)(
            (done_save.total_tries_used * 10) / done_save.puzzles_solved_total);
        sprintf(buf, "AVG TRIES: %d.%d",
                (int)(avg_x10 / 10), (int)(avg_x10 % 10));
        render_text(1, 8, buf);
    }

    render_text(2, 14, "START -> TITLE");
}

static void done_update(Scene *next_scene) {
    if (input_pressed(BTN_START) || input_pressed(BTN_A)) {
        // Cycle restart: reset puzzle pointer to 0, preserve lifetime stats.
        // Lifetime stats kept: puzzles_solved_total, puzzles_skipped_total,
        // current_streak, best_streak, total_tries_used.
        // ip_* are already zero (no in-progress at ALL_DONE).
        done_save.current_puzzle_index = 0;
        done_save.current_puzzle_fails = 0;
        save_store(&done_save);
        sfx_select();
        *next_scene = SCENE_TITLE;
    }
}

static void done_teardown(void) {
}

const SceneVTable SCENE_ALL_DONE_VTABLE = {
    .init = done_init,
    .update = done_update,
    .render = done_render,
    .teardown = done_teardown,
};
