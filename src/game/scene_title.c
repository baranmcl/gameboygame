#include "scene.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include <stdio.h>
#include <stdbool.h>

typedef enum {
    TITLE_MENU_CONTINUE = 0,
    TITLE_MENU_RESTART,
    TITLE_MENU_NEW_GAME,
    TITLE_MENU_COUNT
} TitleMenuItem;

typedef struct {
    GameSave save;
    bool     has_in_progress;
    uint8_t  menu_cursor;
    uint8_t  menu_item_count;   // 2 if no in-progress, 3 if in-progress
    bool     show_confirm;      // NEW GAME confirm overlay
} TitleState;

static TitleState ts;

static void title_init(void) {
    save_load(&ts.save);
    ts.has_in_progress = (ts.save.ip_tries_remaining > 0);
    ts.menu_cursor = 0;
    ts.menu_item_count = ts.has_in_progress ? 3 : 2;
    ts.show_confirm = false;
    // Default tile layout (font + UI) is already loaded by render_init()
    // from main.c. Plan B's TITLE scene is text-only; Plan C will load
    // title.png art via dynamic VRAM swapping.
}

static void title_render(void) {
    render_clear();
    render_text(7, 2, "GBCX");
    render_text(4, 4, "CONNECTIONS");

    if (ts.show_confirm) {
        render_text(2, 8,  "NEW GAME?");
        render_text(2, 10, "ALL DATA LOST");
        render_text(2, 13, "A  CONFIRM");
        render_text(2, 14, "B  CANCEL");
        return;
    }

    char buf[21];
    uint8_t row = 9;
    if (ts.has_in_progress) {
        sprintf(buf, "%cCONTINUE P%d", ts.menu_cursor == 0 ? '>' : ' ', ts.save.current_puzzle_index + 1);
        render_text(2, row + 0, buf);
        sprintf(buf, "%cRESTART P%d",  ts.menu_cursor == 1 ? '>' : ' ', ts.save.current_puzzle_index + 1);
        render_text(2, row + 1, buf);
        sprintf(buf, "%cNEW GAME",     ts.menu_cursor == 2 ? '>' : ' ');
        render_text(2, row + 2, buf);
    } else {
        sprintf(buf, "%cCONTINUE",     ts.menu_cursor == 0 ? '>' : ' ');
        render_text(2, row + 0, buf);
        sprintf(buf, "%cNEW GAME",     ts.menu_cursor == 1 ? '>' : ' ');
        render_text(2, row + 1, buf);
    }

    sprintf(buf, "SOLVED:%d  BEST:%d", ts.save.puzzles_solved_total, ts.save.best_streak);
    render_text(2, 16, buf);
}

static void title_update(Scene *next_scene) {
    if (ts.show_confirm) {
        if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
            // Confirm NEW GAME — wipe save, transition to PLAY
            save_reset(&ts.save);
            save_store(&ts.save);
            sfx_select();
            *next_scene = SCENE_PLAY;
        } else if (input_pressed(BTN_B)) {
            ts.show_confirm = false;
            sfx_deselect();
        }
        return;
    }

    // Menu navigation
    if (input_repeat(BTN_UP)) {
        ts.menu_cursor = (uint8_t)((ts.menu_cursor + ts.menu_item_count - 1) % ts.menu_item_count);
        sfx_move();
    }
    if (input_repeat(BTN_DOWN)) {
        ts.menu_cursor = (uint8_t)((ts.menu_cursor + 1) % ts.menu_item_count);
        sfx_move();
    }

    // Selection
    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
        if (ts.has_in_progress) {
            if (ts.menu_cursor == 0) {
                sfx_select();
                *next_scene = SCENE_PLAY;
            } else if (ts.menu_cursor == 1) {
                // RESTART — clear in-progress fields, keep current_puzzle_index + lifetime stats
                ts.save.ip_tries_remaining = 0;
                ts.save.ip_groups_solved = 0;
                ts.save.ip_selected_mask = 0;
                ts.save.ip_elapsed_seconds = 0;
                save_store(&ts.save);
                sfx_select();
                *next_scene = SCENE_PLAY;
            } else {
                ts.show_confirm = true;
                sfx_select();
            }
        } else {
            if (ts.menu_cursor == 0) {
                sfx_select();
                *next_scene = SCENE_PLAY;
            } else {
                ts.show_confirm = true;
                sfx_select();
            }
        }
    }
}

static void title_teardown(void) {
}

const SceneVTable SCENE_TITLE_VTABLE = {
    .init = title_init,
    .update = title_update,
    .render = title_render,
    .teardown = title_teardown,
};
