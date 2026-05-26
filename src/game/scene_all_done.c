#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "all_done_theme.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include "../engine/music.h"
#include <gb/gb.h>
#include <stdio.h>
#include <stdbool.h>

static GameSave done_save;
static bool     redraw_needed;
// v1.2 Phase 8: scrolling header support. scroll_offset advances every
// SCROLL_FRAMES frames, wrapping at SCREEN_TILES_W. Each tick triggers
// a redraw of the header row only (incremental — static stats below
// untouched).
static uint8_t  scroll_offset;
static uint16_t scroll_last_frame;
static char     scroll_text[21];   // padded "YOU SOLVED ALL N!     " to 20 chars
static uint8_t  scroll_text_len;
#define SCROLL_FRAMES 12           // ~5 col/sec at 60fps

extern volatile uint16_t global_frame_count;

static void done_paint_header(void);

static void done_init(void) {
    BGP_REG = 0xE4;  // defensive: see comment in scene_play's play_init
    save_load(&done_save);

    // v1.2 Phase 8: prepare the scrolling header text once at init.
    // Pad to full screen width (20 chars) so the scroll wraps cleanly
    // without exposing whatever was previously in the tilemap.
    sprintf(scroll_text, "YOU SOLVED ALL %d!", (int)NUM_PUZZLES);
    scroll_text_len = 0;
    while (scroll_text[scroll_text_len] && scroll_text_len < 20) scroll_text_len++;
    // Pad with spaces to 20 chars
    while (scroll_text_len < 20) {
        scroll_text[scroll_text_len++] = ' ';
    }
    scroll_text[20] = 0;
    scroll_offset = 0;
    scroll_last_frame = global_frame_count;

    redraw_needed = true;

    // v1.2 Phase 8: longer 2-voice fanfare instead of the per-puzzle
    // sfx_win. Plays once (non-looping); silence after the final held
    // note is also part of the moment.
    music_play(&ALL_DONE_FANFARE);
}

static void done_paint_header(void) {
    // Render the scrolling header text on row 1. For each character i
    // of the 20-char string, paint at column (scroll_offset + i) % 20.
    // This produces a horizontal scroll that wraps off the right edge
    // and reappears on the left.
    for (uint8_t i = 0; i < 20; i++) {
        uint8_t col = (uint8_t)((scroll_offset + i) % 20);
        char glyph[2];
        glyph[0] = scroll_text[i];
        glyph[1] = 0;
        render_text(col, 1, glyph);
    }
}

static void done_render(void) {
    if (!redraw_needed) return;
    redraw_needed = false;

    render_clear();

    done_paint_header();

    char buf[21];
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
    // v1.2 Phase 8: advance the scrolling header. Frame-counter driven
    // so it stays at the right cadence regardless of main-loop speed.
    if ((uint16_t)(global_frame_count - scroll_last_frame) >= SCROLL_FRAMES) {
        scroll_offset = (uint8_t)((scroll_offset + 1) % 20);
        scroll_last_frame = (uint16_t)(scroll_last_frame + SCROLL_FRAMES);
        redraw_needed = true;
    }

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
    // v1.2 Phase 8: stop the fanfare if player exits before it finishes.
    // music_stop is idempotent; safe to call even if track already ended.
    music_stop();
}

const SceneVTable SCENE_ALL_DONE_VTABLE = {
    .init = done_init,
    .update = done_update,
    .render = done_render,
    .teardown = done_teardown,
};
