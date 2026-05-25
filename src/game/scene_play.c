#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "puzzle_logic.h"
#include "layout.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include "../engine/anim.h"
#include "../assets_gen/ui_tiles.h"
#include <gb/gb.h>
#include <stdio.h>
#include <stdbool.h>

#define CURSOR_SPRITE_INDEX 0

static PlayState  ps;
static GameSave   pg_save;
static LayoutInfo pg_layout;
static bool       redraw_needed = true;   // set on every state change; render() no-ops otherwise

extern volatile uint16_t global_frame_count;

static void update_cursor_sprite(void) {
    // For Phase 5 (groups_solved == 0), slot index == cursor_idx.
    // Phase 6 will refine this when solved-group cells get skipped.
    //
    // Cursor sits on the same row as the cell's word text (cell_y +
    // cell_h/2), not the top-left of the cell — otherwise as cell_h
    // grows after solves the cursor and text would drift apart.
    uint8_t slot = ps.cursor_idx;
    uint8_t tile_x = pg_layout.cell_x[slot];
    uint8_t tile_y = (uint8_t)(pg_layout.cell_y[slot] + pg_layout.cell_h / 2);
    move_sprite(CURSOR_SPRITE_INDEX,
                (uint8_t)(tile_x * 8 + 8),
                (uint8_t)(tile_y * 8 + 16));
}

static void render_cell(const Puzzle *puzzle, uint8_t cell_idx) {
    // Phase 5: identity mapping (groups_solved == 0 → slot == cell_idx).
    // Phase 6 introduces cell_idx_to_slot() when solved-group cells get skipped.
    uint8_t slot = cell_idx;
    if (slot >= pg_layout.cells_count) return;

    uint8_t x = pg_layout.cell_x[slot];
    uint8_t y = pg_layout.cell_y[slot];
    uint8_t selected = (ps.selected_mask & (1u << cell_idx)) != 0;

    uint8_t fill = selected ? UI_TILE_FILL_SEL : UI_TILE_FILL_LIGHT;
    for (uint8_t dy = 0; dy < pg_layout.cell_h; dy++) {
        for (uint8_t dx = 0; dx < 9; dx++) {
            render_set_tile((uint8_t)(x + dx), (uint8_t)(y + dy), fill);
        }
    }

    // Center the word horizontally within the cell. Cell_w = 9 tiles;
    // word_len at most 8 chars → text_x offset = (9 - word_len) / 2.
    const char *word = puzzle->words[cell_idx];
    uint8_t word_len = 0;
    while (word[word_len] && word_len < 9) word_len++;
    uint8_t text_x = (uint8_t)(x + (9 - word_len) / 2);
    uint8_t text_y = (uint8_t)(y + pg_layout.cell_h / 2);
    render_text(text_x, text_y, word);
}

static void play_init(void) {
    save_load(&pg_save);

    // Restore in-progress state if any, else fresh attempt.
    if (pg_save.ip_tries_remaining > 0) {
        ps.tries_remaining = pg_save.ip_tries_remaining;
        ps.groups_solved   = pg_save.ip_groups_solved;
        ps.selected_mask   = pg_save.ip_selected_mask;
        ps.elapsed_seconds = pg_save.ip_elapsed_seconds;
    } else {
        ps.tries_remaining = 4;
        ps.groups_solved   = 0;
        ps.selected_mask   = 0;
        ps.elapsed_seconds = 0;
    }
    ps.cursor_idx = 0;
    ps.show_quit_confirm = 0;

    compute_play_layout(ps.groups_solved, &pg_layout);
    redraw_needed = true;

    // Load the cursor tile into SPRITE VRAM (separate addressable region
    // from background VRAM despite physical overlap on DMG). Without this
    // call, sprites would render whatever bytes happen to be at the sprite
    // tile slot — typically zero-init or stale.
    //
    // ui_tiles_tiles[] starts with the cursor tile (16 bytes). We copy
    // exactly that one tile into sprite slot UI_TILE_CURSOR.
    set_sprite_data(UI_TILE_CURSOR, 1, ui_tiles_tiles);
    set_sprite_tile(CURSOR_SPRITE_INDEX, UI_TILE_CURSOR);
    update_cursor_sprite();
}

static void play_render(void) {
    // Sparse render: only rebuild the tilemap on actual state changes.
    // Without this, calling render_clear + 360 tile writes every frame
    // races with the VBlank ISR's render_flush — half the buffer can get
    // pushed mid-rewrite, causing visible flicker + partial cells.
    if (!redraw_needed) return;
    redraw_needed = false;

    render_clear();

    const Puzzle *puzzle = &PUZZLES[pg_save.current_puzzle_index];

    // Header
    char hdr[21];
    sprintf(hdr, "P%d  TRIES:%d", pg_save.current_puzzle_index + 1, ps.tries_remaining);
    render_text(0, 0, hdr);

    // Draw all 16 cells (Phase 5: assumes groups_solved == 0)
    for (uint8_t i = 0; i < 16; i++) {
        uint8_t g = find_group_of_word(puzzle, i);
        if (ps.groups_solved & (1u << g)) continue;
        render_cell(puzzle, i);
    }

    if (ps.show_quit_confirm) {
        for (uint8_t y = 6; y < 12; y++) {
            for (uint8_t x = 2; x < 18; x++) {
                render_set_tile(x, y, UI_TILE_FILL_LIGHT);
            }
        }
        render_text(4, 7, "QUIT TO TITLE?");
        render_text(4, 9, "A  YES");
        render_text(4, 10, "B  NO");
    }
}

static void play_update(Scene *next_scene) {
    // Cursor blink: toggle sprite visibility every 30 frames
    if ((global_frame_count / 30) & 1) {
        move_sprite(CURSOR_SPRITE_INDEX, 0, 0);  // hide off-screen
    } else {
        update_cursor_sprite();  // show at current cursor position
    }

    if (anim_is_playing()) return;  // input locked during animations

    if (ps.show_quit_confirm) {
        if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
            // Save in-progress state so CONTINUE works on next boot
            pg_save.ip_tries_remaining = ps.tries_remaining;
            pg_save.ip_groups_solved   = ps.groups_solved;
            pg_save.ip_selected_mask   = ps.selected_mask;
            pg_save.ip_elapsed_seconds = ps.elapsed_seconds;
            save_store(&pg_save);
            sfx_select();
            *next_scene = SCENE_TITLE;
        } else if (input_pressed(BTN_B)) {
            ps.show_quit_confirm = 0;
            redraw_needed = true;
            sfx_deselect();
        }
        return;
    }

    // Cursor nav (auto-repeat). For Phase 5 (no solved groups), the
    // skip-solved-cell loop in the plan terminates immediately on every
    // entry — kept here for compatibility with Phase 6 where groups_solved
    // can be non-zero.
    if (input_repeat(BTN_UP)) {
        uint8_t row = ps.cursor_idx / 2, col = ps.cursor_idx % 2;
        row = (uint8_t)((row + 7) % 8);
        ps.cursor_idx = (uint8_t)(row * 2 + col);
        update_cursor_sprite();
        sfx_move();
    }
    if (input_repeat(BTN_DOWN)) {
        uint8_t row = ps.cursor_idx / 2, col = ps.cursor_idx % 2;
        row = (uint8_t)((row + 1) % 8);
        ps.cursor_idx = (uint8_t)(row * 2 + col);
        update_cursor_sprite();
        sfx_move();
    }
    if (input_pressed(BTN_LEFT)) {
        if (ps.cursor_idx % 2 == 1) {
            ps.cursor_idx--;
            update_cursor_sprite();
            sfx_move();
        }
    }
    if (input_pressed(BTN_RIGHT)) {
        if (ps.cursor_idx % 2 == 0) {
            ps.cursor_idx++;
            update_cursor_sprite();
            sfx_move();
        }
    }

    // Selection toggle on A
    if (input_pressed(BTN_A)) {
        if (toggle_selection(&ps, ps.cursor_idx)) {
            bool is_now_set = (ps.selected_mask & (1u << ps.cursor_idx)) != 0;
            uint8_t data[8] = { ps.cursor_idx };
            anim_start(ANIM_SELECT_FLASH, data, 4);
            redraw_needed = true;
            if (is_now_set) sfx_select(); else sfx_deselect();
        } else {
            sfx_reject();  // tried to select a 5th
        }
    }

    // B clears all selections
    if (input_pressed(BTN_B)) {
        if (ps.selected_mask != 0) {
            ps.selected_mask = 0;
            redraw_needed = true;
            sfx_deselect();
        }
    }

    // SELECT triggers quit confirm
    if (input_pressed(BTN_SELECT)) {
        ps.show_quit_confirm = 1;
        redraw_needed = true;
        sfx_select();
    }

    // Phase 5 stub: START is wired in Phase 6 (submission logic).
    // For now, START does nothing.
    (void)next_scene;  // unused in Phase 5 (no transitions yet)
}

static void play_teardown(void) {
    // Hide cursor sprite when leaving PLAY
    move_sprite(CURSOR_SPRITE_INDEX, 0, 0);
}

const SceneVTable SCENE_PLAY_VTABLE = {
    .init = play_init,
    .update = play_update,
    .render = play_render,
    .teardown = play_teardown,
};
