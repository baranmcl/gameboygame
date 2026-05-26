#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "scene_handoff.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include "../assets_gen/ui_tiles.h"
#include <gb/gb.h>
#include <stdio.h>
#include <stdbool.h>

// v1.2 Phase 7: puzzle-select scene.
//
// Grid layout: 5 columns × up to 10 rows. Each puzzle cell is 2 tiles
// wide × 2 tiles tall + 1-tile padding between cells. Origin row 3
// (leaves rows 0-2 for the PUZZLES header), origin col 2 (centers
// 5-cell-wide grid). Bottom rows show focused-puzzle stats + input
// hints.
//
// Display rules:
//   - Completed puzzle: dark-fill cell, palette cycled by puzzle_idx
//     so the grid has a multi-colored mosaic feel on GBC.
//   - Not-yet-completed: light-fill cell, default palette.
//   - Cursor: SEL-fill (selected highlight color), overrides
//     completed/not-completed coloring for the focused cell.

static GameSave  ps_save;
static uint8_t   ps_cursor;            // 0..NUM_PUZZLES-1
static bool      ps_redraw_needed;

#define GRID_COL_COUNT     5
#define GRID_CELL_W        3   // 2 tile cell + 1 tile padding to next
#define GRID_CELL_H        2   // 1 tile cell + 1 tile padding to next
#define GRID_ORIGIN_X      2
#define GRID_ORIGIN_Y      3

// Forward decls — helper bodies after the vtable.
static void ps_render_grid_cell(uint8_t puzzle_idx, bool is_cursor);
static void ps_render_focused_stats(void);

static void ps_init(void) {
    BGP_REG = 0xE4;
    save_load(&ps_save);
    ps_cursor = 0;
    ps_redraw_needed = true;
}

static void ps_render(void) {
    if (!ps_redraw_needed) return;
    ps_redraw_needed = false;
    render_clear();

    render_text(7, 0, "PUZZLES");

    for (uint8_t i = 0; i < NUM_PUZZLES; i++) {
        ps_render_grid_cell(i, i == ps_cursor);
    }

    ps_render_focused_stats();

    render_text(0, 17, "A PLAY  B BACK");
}

static void ps_update(Scene *next_scene) {
    uint8_t col = (uint8_t)(ps_cursor % GRID_COL_COUNT);

    // D-pad navigation. LEFT/RIGHT moves one cell with row-edge wrap;
    // UP/DOWN moves by full row. Bounds-clamped to NUM_PUZZLES-1 so
    // navigating off the bottom of a partial row stays in the grid.
    if (input_repeat(BTN_LEFT)) {
        if (ps_cursor > 0) {
            ps_cursor--;
            ps_redraw_needed = true;
            sfx_move();
        }
    }
    if (input_repeat(BTN_RIGHT)) {
        if (ps_cursor + 1 < NUM_PUZZLES) {
            ps_cursor++;
            ps_redraw_needed = true;
            sfx_move();
        }
    }
    if (input_repeat(BTN_UP)) {
        if (ps_cursor >= GRID_COL_COUNT) {
            ps_cursor = (uint8_t)(ps_cursor - GRID_COL_COUNT);
            ps_redraw_needed = true;
            sfx_move();
        }
    }
    if (input_repeat(BTN_DOWN)) {
        uint8_t target = (uint8_t)(ps_cursor + GRID_COL_COUNT);
        if (target < NUM_PUZZLES) {
            ps_cursor = target;
            ps_redraw_needed = true;
            sfx_move();
        }
    }
    (void)col;  // currently only used implicitly by ps_cursor math above

    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
        // Only completed puzzles are replayable. Trying to play an
        // uncompleted one plays sfx_reject and stays on the grid.
        bool is_completed = (ps_save.completed_bits[ps_cursor >> 3]
                          & (uint8_t)(1u << (ps_cursor & 7))) != 0;
        if (is_completed) {
            sfx_select();
            replay_puzzle_index = ps_cursor;
            *next_scene = SCENE_PLAY;
        } else {
            sfx_reject();
        }
    } else if (input_pressed(BTN_B)) {
        sfx_deselect();
        replay_puzzle_index = REPLAY_NONE;
        *next_scene = SCENE_TITLE;
    }
}

static void ps_teardown(void) {
}

// ---- helpers ----

static void ps_render_grid_cell(uint8_t puzzle_idx, bool is_cursor) {
    uint8_t row = (uint8_t)(puzzle_idx / GRID_COL_COUNT);
    uint8_t col = (uint8_t)(puzzle_idx % GRID_COL_COUNT);
    uint8_t x   = (uint8_t)(GRID_ORIGIN_X + col * GRID_CELL_W);
    uint8_t y   = (uint8_t)(GRID_ORIGIN_Y + row * GRID_CELL_H);

    bool is_completed = (ps_save.completed_bits[puzzle_idx >> 3]
                      & (uint8_t)(1u << (puzzle_idx & 7))) != 0;

    uint8_t body_tile;
    if (is_cursor) {
        body_tile = UI_TILE_FILL_SEL;
    } else if (is_completed) {
        body_tile = UI_TILE_SOLID_DARK;
    } else {
        body_tile = UI_TILE_FILL_LIGHT;
    }

    for (uint8_t dx = 0; dx < 2; dx++) {
        for (uint8_t dy = 0; dy < 1; dy++) {
            render_set_tile((uint8_t)(x + dx), (uint8_t)(y + dy), body_tile);
        }
    }

    // GBC: color completed cells with a tier palette cycling by puzzle
    // index — gives the grid a multi-colored mosaic feel rather than
    // a uniform dark block. No-op on DMG.
    if (is_completed && !is_cursor) {
        uint8_t palette = (uint8_t)(GBC_PAL_TIER_YELLOW + (puzzle_idx % 4));
        for (uint8_t dx = 0; dx < 2; dx++) {
            for (uint8_t dy = 0; dy < 1; dy++) {
                render_set_tile_palette((uint8_t)(x + dx), (uint8_t)(y + dy), palette);
            }
        }
    }
}

static void ps_render_focused_stats(void) {
    char buf[21];
    uint8_t p = ps_cursor;
    sprintf(buf, "PUZZLE %d", (int)(p + 1));
    render_text(2, 14, buf);

    bool is_completed = (ps_save.completed_bits[p >> 3]
                      & (uint8_t)(1u << (p & 7))) != 0;
    if (is_completed) {
        uint16_t bt = ps_save.puzzle_best_time[p];
        uint8_t  bg = ps_save.puzzle_best_tries[p];
        // best_time is the SOLE sentinel for "no record" — best_tries
        // can legitimately be 0 (perfect game with no wrong submissions).
        // best_time can't realistically be 0 since elapsed_seconds
        // increments every 60 frames and reading/selecting 4 words
        // takes longer than 1/60s.
        if (bt == 0) {
            render_text(2, 15, "BEST: --");
        } else {
            uint16_t mins = (uint16_t)(bt / 60);
            uint16_t secs = (uint16_t)(bt % 60);
            sprintf(buf, "BEST %d:%02d  %d/4",
                    (int)mins, (int)secs, (int)bg);
            render_text(2, 15, buf);
        }
    } else {
        render_text(2, 15, "NOT YET PLAYED");
    }
}

const SceneVTable SCENE_PUZZLE_SELECT_VTABLE = {
    .init = ps_init,
    .update = ps_update,
    .render = ps_render,
    .teardown = ps_teardown,
};
