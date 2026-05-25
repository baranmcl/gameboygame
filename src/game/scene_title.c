#include "scene.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include <gb/gb.h>
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
    bool     has_in_progress;     // ip_tries_remaining > 0
    bool     has_any_progress;    // any history at all (solved/skipped/index)
    uint8_t  menu_cursor;
    uint8_t  menu_item_count;     // 1 (fresh) / 2 (returning) / 3 (mid-puzzle)
    bool     show_confirm;        // NEW GAME confirm overlay
    bool     redraw_needed;       // gates re-render to avoid every-frame flicker
} TitleState;

static TitleState ts;

static void title_init(void) {
    BGP_REG = 0xE4;  // defensive: see comment in scene_play's play_init
    save_load(&ts.save);
    ts.has_in_progress = (ts.save.ip_tries_remaining > 0);
    // "Any progress" = anything that would make CONTINUE meaningful.
    // On a truly-fresh save (never played), CONTINUE and NEW GAME would
    // do the same thing, so we hide CONTINUE and show only NEW GAME.
    ts.has_any_progress = ts.has_in_progress
        || (ts.save.current_puzzle_index > 0)
        || (ts.save.puzzles_solved_total > 0)
        || (ts.save.puzzles_skipped_total > 0);

    ts.menu_cursor = 0;
    if (ts.has_in_progress) {
        ts.menu_item_count = 3;   // CONTINUE / RESTART / NEW GAME
    } else if (ts.has_any_progress) {
        ts.menu_item_count = 2;   // CONTINUE / NEW GAME
    } else {
        ts.menu_item_count = 1;   // NEW GAME only
    }
    ts.show_confirm = false;
    ts.redraw_needed = true;
    // Default tile layout (font + UI) is already loaded by render_init()
    // from main.c. Plan B's TITLE scene is text-only; Plan C will load
    // title.png art via dynamic VRAM swapping.
}

static void title_render(void) {
    if (!ts.redraw_needed) return;
    ts.redraw_needed = false;

    render_clear();
    // Title banner: 8 tiles wide × 4 tall, centered horizontally on
    // a 20-tile screen → starts at column (20-8)/2 = 6. Tiles in
    // title.png are arranged row-major so banner tile (r, c) lives at
    // TITLE_TILE_BASE + r * TITLE_BANNER_W + c.
    {
        uint8_t banner_x = (uint8_t)((SCREEN_TILES_W - TITLE_BANNER_W) / 2);
        uint8_t banner_y = 1;  // rows 1-4 → leaves row 0 + row 5+ free
        for (uint8_t r = 0; r < TITLE_BANNER_H; r++) {
            for (uint8_t c = 0; c < TITLE_BANNER_W; c++) {
                uint8_t tile_idx = (uint8_t)(r * TITLE_BANNER_W + c);
                render_set_tile((uint8_t)(banner_x + c),
                                (uint8_t)(banner_y + r),
                                (uint8_t)(TITLE_TILE_BASE + tile_idx));
            }
        }
    }
    render_text(4, 6, "CONNECTIONS");

    if (ts.show_confirm) {
        render_text(2, 8,  "NEW GAME?");
        render_text(2, 10, "ALL DATA LOST");
        render_text(2, 13, "A  CONFIRM");
        render_text(2, 14, "B  CANCEL");
        return;
    }

    char buf[21];
    uint8_t row = 9;
    int idx_plus_1 = (int)ts.save.current_puzzle_index + 1;

    // SDCC's sprintf has a varargs-alignment bug: when %c is followed by
    // other format specs, the second-and-later args read from skewed byte
    // offsets, producing garbage values (e.g. value 1 reads as 256 =
    // 0x0100, a 1-byte high-byte shift). Workaround: write the cursor
    // marker as a plain char into buf[0], then sprintf into buf+1.
    if (ts.has_in_progress) {
        buf[0] = (ts.menu_cursor == 0) ? '>' : ' ';
        sprintf(&buf[1], "CONTINUE P%d", idx_plus_1);
        render_text(2, row + 0, buf);

        buf[0] = (ts.menu_cursor == 1) ? '>' : ' ';
        sprintf(&buf[1], "RESTART P%d", idx_plus_1);
        render_text(2, row + 1, buf);

        buf[0] = (ts.menu_cursor == 2) ? '>' : ' ';
        buf[1] = 'N'; buf[2] = 'E'; buf[3] = 'W'; buf[4] = ' ';
        buf[5] = 'G'; buf[6] = 'A'; buf[7] = 'M'; buf[8] = 'E'; buf[9] = 0;
        render_text(2, row + 2, buf);
    } else if (ts.has_any_progress) {
        buf[0] = (ts.menu_cursor == 0) ? '>' : ' ';
        buf[1] = 'C'; buf[2] = 'O'; buf[3] = 'N'; buf[4] = 'T'; buf[5] = 'I';
        buf[6] = 'N'; buf[7] = 'U'; buf[8] = 'E'; buf[9] = 0;
        render_text(2, row + 0, buf);

        buf[0] = (ts.menu_cursor == 1) ? '>' : ' ';
        buf[1] = 'N'; buf[2] = 'E'; buf[3] = 'W'; buf[4] = ' ';
        buf[5] = 'G'; buf[6] = 'A'; buf[7] = 'M'; buf[8] = 'E'; buf[9] = 0;
        render_text(2, row + 1, buf);
    } else {
        buf[0] = (ts.menu_cursor == 0) ? '>' : ' ';
        buf[1] = 'N'; buf[2] = 'E'; buf[3] = 'W'; buf[4] = ' ';
        buf[5] = 'G'; buf[6] = 'A'; buf[7] = 'M'; buf[8] = 'E'; buf[9] = 0;
        render_text(2, row + 0, buf);
    }

    sprintf(buf, "SOLVED:%d  BEST:%d",
            (int)ts.save.puzzles_solved_total,
            (int)ts.save.best_streak);
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
            ts.redraw_needed = true;
            sfx_deselect();
        }
        return;
    }

    // Menu navigation
    if (ts.menu_item_count > 1) {
        if (input_repeat(BTN_UP)) {
            ts.menu_cursor = (uint8_t)((ts.menu_cursor + ts.menu_item_count - 1) % ts.menu_item_count);
            ts.redraw_needed = true;
            sfx_move();
        }
        if (input_repeat(BTN_DOWN)) {
            ts.menu_cursor = (uint8_t)((ts.menu_cursor + 1) % ts.menu_item_count);
            ts.redraw_needed = true;
            sfx_move();
        }
    }

    // Selection
    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
        if (ts.has_in_progress) {
            // 3-item menu: CONTINUE / RESTART / NEW GAME
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
                ts.redraw_needed = true;
                sfx_select();
            }
        } else if (ts.has_any_progress) {
            // 2-item menu: CONTINUE / NEW GAME
            if (ts.menu_cursor == 0) {
                sfx_select();
                *next_scene = SCENE_PLAY;
            } else {
                ts.show_confirm = true;
                ts.redraw_needed = true;
                sfx_select();
            }
        } else {
            // 1-item menu: NEW GAME only — no progress to lose, so skip
            // the confirm overlay. save_reset is harmless on already-empty
            // save (idempotent).
            save_reset(&ts.save);
            save_store(&ts.save);
            sfx_select();
            *next_scene = SCENE_PLAY;
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
