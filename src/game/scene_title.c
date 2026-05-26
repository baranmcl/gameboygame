#include "scene.h"
#include "title_theme.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "scene_handoff.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include "../engine/music.h"
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
    bool     has_any_completed;   // v1.2: any bit in completed_bits set
    uint8_t  menu_cursor;
    uint8_t  menu_item_count;     // 1 (fresh) / 2 (returning) / 3 (mid-puzzle) +1 each if PUZZLES visible
    uint8_t  puzzles_cursor_idx;  // v1.2: menu_cursor value that selects PUZZLES (0xFF if hidden)
    bool     show_confirm;        // NEW GAME confirm overlay
    bool     last_rendered_show_confirm;  // v1.2 Phase 7 fix: track dialog state for incremental render
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

    // v1.2 Phase 7: PUZZLES menu option visible iff any bit in
    // completed_bits is set. Requires has_any_progress (you can't have
    // a completed puzzle without progress), so the fresh-save path
    // never shows PUZZLES.
    ts.has_any_completed = false;
    for (uint8_t i = 0; i < (MAX_PUZZLES_SUPPORTED / 8); i++) {
        if (ts.save.completed_bits[i] != 0) {
            ts.has_any_completed = true;
            break;
        }
    }

    ts.menu_cursor = 0;
    if (ts.has_in_progress) {
        ts.menu_item_count = 3;   // CONTINUE / RESTART / NEW GAME
    } else if (ts.has_any_progress) {
        ts.menu_item_count = 2;   // CONTINUE / NEW GAME
    } else {
        ts.menu_item_count = 1;   // NEW GAME only
    }
    // PUZZLES is always the LAST item if visible. Its cursor index is
    // the current item count; then bump item count by 1.
    if (ts.has_any_completed) {
        ts.puzzles_cursor_idx = ts.menu_item_count;
        ts.menu_item_count++;
    } else {
        ts.puzzles_cursor_idx = 0xFF;
    }

    ts.show_confirm = false;
    ts.last_rendered_show_confirm = false;  // matches initial show_confirm
    ts.redraw_needed = true;
    // Default tile layout (font + UI) is already loaded by render_init()
    // from main.c. Plan B's TITLE scene is text-only; Plan C will load
    // title.png art via dynamic VRAM swapping.

    // v1.2 Phase 7 fix: clear ONCE here. Subsequent menu-cycle redraws
    // skip render_clear (see title_render) to avoid VBlank-racing
    // flicker — the static chrome (banner, subtitle, attribution,
    // stats) just gets overwritten with itself; the menu cursor diff
    // overwrites cleanly in-place.
    render_clear();

    // Plan D Phase 6: start the upbeat 8-bar title theme on CH1.
    // SFX continue using CH2/CH4 (sfx_move/select/deselect on CH2,
    // sfx_reject on CH4) so they don't conflict with the music.
    music_play(&TITLE_THEME);
}

static void title_render(void) {
    if (!ts.redraw_needed) return;
    ts.redraw_needed = false;

    // v1.2 Phase 7 fix: only call render_clear when the dialog state
    // toggled (open or close), since the dialog overlay can leave
    // stale text on rows 8/10/13/14 that need wiping. For pure menu-
    // cursor cycling, the existing tile writes overwrite themselves
    // (banner/subtitle/attribution unchanged) or the new cursor state
    // (menu rows). No clear → no VBlank race → no flicker.
    if (ts.show_confirm != ts.last_rendered_show_confirm) {
        render_clear();
        ts.last_rendered_show_confirm = ts.show_confirm;
    }
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

    // Inconspicuous attribution at the bottom — rendered before the
    // show_confirm return so it appears on both the main menu and the
    // NEW GAME confirm overlay. 20 chars at col 0 = exact fit, no
    // margin needed.
    render_text(0, 17, "BY MCLEAN BARAN 2026");

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

    // v1.2 Phase 7: PUZZLES menu line, displayed below the existing
    // items. Its row depends on how many items appeared above it,
    // captured at init time as puzzles_cursor_idx (= row offset).
    if (ts.has_any_completed) {
        buf[0] = (ts.menu_cursor == ts.puzzles_cursor_idx) ? '>' : ' ';
        buf[1] = 'P'; buf[2] = 'U'; buf[3] = 'Z'; buf[4] = 'Z'; buf[5] = 'L';
        buf[6] = 'E'; buf[7] = 'S'; buf[8] = 0;
        render_text(2, (uint8_t)(row + ts.puzzles_cursor_idx), buf);
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
        // v1.2 Phase 7: PUZZLES handler — checked first since it sits
        // at the highest cursor index in every menu variant.
        if (ts.has_any_completed && ts.menu_cursor == ts.puzzles_cursor_idx) {
            sfx_select();
            replay_puzzle_index = REPLAY_NONE;  // defensive: ensure clean state
            *next_scene = SCENE_PUZZLE_SELECT;
            return;
        }

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
    music_stop();
}

const SceneVTable SCENE_TITLE_VTABLE = {
    .init = title_init,
    .update = title_update,
    .render = title_render,
    .teardown = title_teardown,
};
