#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "puzzle_logic.h"
#include "layout.h"
#include "scene_handoff.h"
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
static bool       redraw_needed = true;
static uint16_t   last_second_frame = 0;

extern volatile uint16_t global_frame_count;

// Map an absolute cell index (0..15) to its current layout slot.
// When groups are solved, those 4 cells "disappear" and the remaining
// unsolved cells get re-packed into pg_layout.cell_x/y entries 0..N-1.
// Walk cells [0..cell_idx) and count how many are still unsolved — that
// count is cell_idx's slot.
static uint8_t cell_idx_to_slot(const Puzzle *puzzle, uint8_t cell_idx) {
    uint8_t slot = 0;
    for (uint8_t i = 0; i < cell_idx; i++) {
        uint8_t g = find_group_of_word(puzzle, i);
        if (!(ps.groups_solved & (1u << g))) slot++;
    }
    return slot;
}

static bool cell_idx_is_solved(const Puzzle *puzzle, uint8_t cell_idx) {
    uint8_t g = find_group_of_word(puzzle, cell_idx);
    return (ps.groups_solved & (1u << g)) != 0;
}

// Inverse of cell_idx_to_slot: given a slot in the current layout
// (0..cells_count-1), return the absolute cell index (0..15).
static uint8_t slot_to_cell_idx(const Puzzle *puzzle, uint8_t slot) {
    uint8_t s = 0;
    for (uint8_t i = 0; i < 16; i++) {
        if (cell_idx_is_solved(puzzle, i)) continue;
        if (s == slot) return i;
        s++;
    }
    return 0;  // shouldn't happen if slot < cells_count
}

static void update_cursor_sprite(void) {
    // If cursor lands on an already-solved cell (e.g., after a correct
    // submission), hide the sprite — caller should reposition cursor
    // before next render.
    const Puzzle *puzzle = &PUZZLES[pg_save.current_puzzle_index];
    if (cell_idx_is_solved(puzzle, ps.cursor_idx)) {
        move_sprite(CURSOR_SPRITE_INDEX, 0, 0);
        return;
    }
    uint8_t slot = cell_idx_to_slot(puzzle, ps.cursor_idx);
    uint8_t tile_x = pg_layout.cell_x[slot];
    uint8_t tile_y = (uint8_t)(pg_layout.cell_y[slot] + pg_layout.cell_h / 2);
    move_sprite(CURSOR_SPRITE_INDEX,
                (uint8_t)(tile_x * 8 + 8),
                (uint8_t)(tile_y * 8 + 16));
}

// Move cursor to the first unsolved cell starting from current cursor_idx
// (wrapping). Called after a correct group solve to skip over the just-
// solved cells.
static void reposition_cursor_to_unsolved(const Puzzle *puzzle) {
    for (uint8_t tries = 0; tries < 16; tries++) {
        uint8_t check = (uint8_t)((ps.cursor_idx + tries) % 16);
        if (!cell_idx_is_solved(puzzle, check)) {
            ps.cursor_idx = check;
            return;
        }
    }
    // All 16 cells solved — caller should have transitioned to WIN already.
    ps.cursor_idx = 0;
}

static void render_cell(const Puzzle *puzzle, uint8_t cell_idx) {
    uint8_t slot = cell_idx_to_slot(puzzle, cell_idx);
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

    const char *word = puzzle->words[cell_idx];
    uint8_t word_len = 0;
    while (word[word_len] && word_len < 9) word_len++;
    uint8_t text_x = (uint8_t)(x + (9 - word_len) / 2);
    uint8_t text_y = (uint8_t)(y + pg_layout.cell_h / 2);
    render_text(text_x, text_y, word);
}

static void render_solved_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y) {
    // Bar layout: [3 tier-pattern tiles] [14 solid-dark label tiles] [3 tier-pattern]
    // Category name overlays the label using the inverted font tiles
    // (light glyphs on dark — Plan C addition).
    uint8_t pattern_tile = (uint8_t)(UI_TILE_PATTERN_BASE + tier);
    for (uint8_t x = 0; x < 3; x++)   render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 17; x < 20; x++) render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 3; x < 17; x++)  render_set_tile(x, y, UI_TILE_SOLID_DARK);

    // Overlay category name centered in the 14-tile label region (cols 3..16)
    const char *name = puzzle->category_names[tier];
    uint8_t name_len = 0;
    while (name[name_len] && name_len < 14) name_len++;
    uint8_t text_x = (uint8_t)(3 + (14 - name_len) / 2);
    render_text_inv(text_x, y, name);

    // Plan D Phase 3: paint the GBC palette index for the whole 20-tile
    // bar row so the tile pattern + solid + text glyphs render in the tier
    // color. tier enum (0=yellow..3=purple) maps directly to palette
    // indices (GBC_PAL_TIER_YELLOW..GBC_PAL_TIER_PURPLE = 1..4).
    // No-op on DMG (render_set_tile_palette is a no-op there).
    uint8_t palette = (uint8_t)(GBC_PAL_TIER_YELLOW + tier);
    for (uint8_t x = 0; x < SCREEN_TILES_W; x++) {
        render_set_tile_palette(x, y, palette);
    }
}

static void play_init(void) {
    // Defensive palette reset. Scenes can be entered mid-animation if a
    // chain transition fires before the prior anim's cleanup runs (e.g.,
    // submission triggers CORRECT_FLASH then immediately transitions to
    // WIN — CORRECT_FLASH's first tick can write BGP=0x1B before WIN's
    // anim_start overwrites the active anim, stranding the inverted
    // palette). Reset to normal at every scene boundary.
    BGP_REG = 0xE4;

    save_load(&pg_save);

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
    // If restoring an in-progress puzzle that already has some groups
    // solved, cursor might land on a solved cell — reposition.
    {
        const Puzzle *puzzle = &PUZZLES[pg_save.current_puzzle_index];
        if (cell_idx_is_solved(puzzle, ps.cursor_idx)) {
            reposition_cursor_to_unsolved(puzzle);
        }
    }

    compute_play_layout(ps.groups_solved, &pg_layout);
    redraw_needed = true;
    last_second_frame = global_frame_count;

    set_sprite_data(UI_TILE_CURSOR, 1, ui_tiles_tiles);
    set_sprite_tile(CURSOR_SPRITE_INDEX, UI_TILE_CURSOR);
    update_cursor_sprite();
}

// Paint the header row. Header is fixed-width ("P<n>  TRIES:<m>"), so
// overwriting always replaces every char that could change.
static void render_header(void) {
    char hdr[21];
    sprintf(hdr, "P%d  TRIES:%d",
            (int)pg_save.current_puzzle_index + 1,
            (int)ps.tries_remaining);
    render_text(0, 0, hdr);
}

// Paint the full board (header + bars + cells). Caller is responsible
// for ensuring the underlying tilemap is clear or in a known state —
// this writes EVERY tile of the active board region, so it's safe to
// overlay onto a previous full board (idempotent), but not safe to
// rely on for restoring after a partial overwrite of empty area.
static void render_board(const Puzzle *puzzle) {
    render_header();
    for (uint8_t i = 0; i < pg_layout.bars_count; i++) {
        render_solved_bar(puzzle, pg_layout.bar_tier[i], (uint8_t)(pg_layout.bar_y[i] + 1));
    }
    for (uint8_t i = 0; i < 16; i++) {
        if (cell_idx_is_solved(puzzle, i)) continue;
        render_cell(puzzle, i);
    }
}

static void play_render(void) {
    if (!redraw_needed) return;
    redraw_needed = false;

    // Full-redraw path. Used for init, layout change after correct
    // submit, and quit-confirm dialog open/close — i.e., cases where
    // the whole tilemap state can shift. We still render_clear here
    // because correct-submit shrinks the layout (cells disappear from
    // bottom rows) and quit-dialog dismissal needs to wipe the dialog
    // box. Single-frame flicker on these transitions is acceptable —
    // they coincide with intentional animations (CORRECT_FLASH) or
    // explicit user UI changes (dialog).
    //
    // Hot-path interactions (A-toggle, B-clear, wrong-submit) bypass
    // this whole function — they call render_cell / render_header
    // directly to avoid the render_clear → partial-flush race that
    // produced visible flicker on every selection toggle.
    render_clear();

    const Puzzle *puzzle = &PUZZLES[pg_save.current_puzzle_index];
    render_board(puzzle);

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

// Helper: write the current in-progress fields to pg_save and save_store.
// Called at every save-trigger point per spec §4.
static void save_in_progress(void) {
    pg_save.ip_tries_remaining = ps.tries_remaining;
    pg_save.ip_groups_solved   = ps.groups_solved;
    pg_save.ip_selected_mask   = ps.selected_mask;
    pg_save.ip_elapsed_seconds = ps.elapsed_seconds;
    save_store(&pg_save);
}

static void play_update(Scene *next_scene) {
    // Tick elapsed_seconds (60Hz frame count → 1Hz second tick)
    if ((uint16_t)(global_frame_count - last_second_frame) >= 60) {
        ps.elapsed_seconds++;
        last_second_frame = (uint16_t)(last_second_frame + 60);
    }

    // Cursor blink (cursor sprite visibility toggle every 30 frames)
    if ((global_frame_count / 30) & 1) {
        move_sprite(CURSOR_SPRITE_INDEX, 0, 0);
    } else {
        update_cursor_sprite();
    }

    if (anim_is_playing()) return;

    if (ps.show_quit_confirm) {
        if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
            save_in_progress();
            sfx_select();
            *next_scene = SCENE_TITLE;
        } else if (input_pressed(BTN_B)) {
            ps.show_quit_confirm = 0;
            redraw_needed = true;
            sfx_deselect();
        }
        return;
    }

    const Puzzle *puzzle = &PUZZLES[pg_save.current_puzzle_index];

    // Cursor nav (slot-based, not absolute cell_idx).
    // After group solves, cells re-pack into a smaller layout (16→12→8→4),
    // so navigating by absolute cell_idx row/col makes the cursor jump
    // across columns in confusing ways. Instead: compute the cursor's
    // current visual slot, advance the slot per d-pad direction, then
    // map back to cell_idx via slot_to_cell_idx().
    uint8_t cur_slot = cell_idx_to_slot(puzzle, ps.cursor_idx);
    uint8_t total_rows = (uint8_t)(pg_layout.cells_count / 2);

    if (input_repeat(BTN_UP) && total_rows > 0) {
        uint8_t col = (uint8_t)(cur_slot % 2);
        uint8_t row = (uint8_t)(cur_slot / 2);
        row = (uint8_t)((row + total_rows - 1) % total_rows);
        uint8_t new_slot = (uint8_t)(row * 2 + col);
        ps.cursor_idx = slot_to_cell_idx(puzzle, new_slot);
        update_cursor_sprite();
        sfx_move();
    }
    if (input_repeat(BTN_DOWN) && total_rows > 0) {
        uint8_t col = (uint8_t)(cur_slot % 2);
        uint8_t row = (uint8_t)(cur_slot / 2);
        row = (uint8_t)((row + 1) % total_rows);
        uint8_t new_slot = (uint8_t)(row * 2 + col);
        ps.cursor_idx = slot_to_cell_idx(puzzle, new_slot);
        update_cursor_sprite();
        sfx_move();
    }
    if (input_pressed(BTN_LEFT)) {
        if (cur_slot % 2 == 1) {
            uint8_t new_slot = (uint8_t)(cur_slot - 1);
            ps.cursor_idx = slot_to_cell_idx(puzzle, new_slot);
            update_cursor_sprite();
            sfx_move();
        }
    }
    if (input_pressed(BTN_RIGHT)) {
        if (cur_slot % 2 == 0 && cur_slot + 1 < pg_layout.cells_count) {
            uint8_t new_slot = (uint8_t)(cur_slot + 1);
            ps.cursor_idx = slot_to_cell_idx(puzzle, new_slot);
            update_cursor_sprite();
            sfx_move();
        }
    }

    // Selection toggle on A — repaint ONLY the toggled cell.
    // Don't set redraw_needed: that path does render_clear which races
    // VBlank and produces visible flicker on every A-press. render_cell
    // overwrites just this cell's ~27 tiles, keeping the rest of the
    // tilemap consistent at every moment.
    //
    // Originally we also fired ANIM_SELECT_FLASH here for "visual
    // punctuation", but that inverts the entire BGP palette for 4
    // frames — a whole-screen flash on every selection. Now that the
    // cell fill swaps directly to UI_TILE_FILL_SEL, the cell-color
    // change IS the feedback. The palette flash was redundant noise.
    if (input_pressed(BTN_A)) {
        if (toggle_selection(&ps, ps.cursor_idx)) {
            bool is_now_set = (ps.selected_mask & (1u << ps.cursor_idx)) != 0;
            render_cell(puzzle, ps.cursor_idx);
            if (is_now_set) sfx_select(); else sfx_deselect();
        } else {
            sfx_reject();
        }
    }

    // B clears all selections — repaint only the cells that were
    // selected (same anti-flicker rationale as A-press above).
    if (input_pressed(BTN_B)) {
        if (ps.selected_mask != 0) {
            uint16_t prev_mask = ps.selected_mask;
            ps.selected_mask = 0;
            for (uint8_t i = 0; i < 16; i++) {
                if (prev_mask & (1u << i)) render_cell(puzzle, i);
            }
            sfx_deselect();
        }
    }

    // SELECT triggers quit confirm
    if (input_pressed(BTN_SELECT)) {
        ps.show_quit_confirm = 1;
        redraw_needed = true;
        sfx_select();
    }

    // START submits the current 4-selection
    if (input_pressed(BTN_START)) {
        if (count_selected(ps.selected_mask) != 4) {
            sfx_reject();
            return;
        }

        if (is_group_correct(puzzle, ps.selected_mask)) {
            // Find which group was just solved (any selected cell's group)
            uint8_t solved_group = 0;
            for (uint8_t i = 0; i < 16; i++) {
                if (ps.selected_mask & (1u << i)) {
                    solved_group = find_group_of_word(puzzle, i);
                    break;
                }
            }
            ps.groups_solved |= (uint8_t)(1u << solved_group);
            ps.selected_mask = 0;

            uint8_t data[8] = { solved_group };
            anim_start(ANIM_CORRECT_FLASH, data, 12);
            sfx_correct();

            // Recompute layout (one fewer group) and reposition cursor
            // off the just-solved cells.
            compute_play_layout(ps.groups_solved, &pg_layout);
            reposition_cursor_to_unsolved(puzzle);
            redraw_needed = true;

            // Save mid-puzzle state for CONTINUE / hard-reset resume
            save_in_progress();

            // All 4 solved → transition to WIN, updating lifetime stats
            if (ps.groups_solved == 0x0F) {
                // Plan C: capture per-puzzle stats BEFORE the save
                // mutations below reset current_puzzle_fails.
                last_puzzle_result.tries_used = (uint8_t)(4 - ps.tries_remaining);
                last_puzzle_result.elapsed_seconds = ps.elapsed_seconds;
                last_puzzle_result.attempt_number =
                    (uint8_t)(pg_save.current_puzzle_fails + 1);

                pg_save.puzzles_solved_total++;
                pg_save.current_streak++;
                if (pg_save.current_streak > pg_save.best_streak) {
                    pg_save.best_streak = pg_save.current_streak;
                }
                pg_save.total_tries_used = (uint16_t)(
                    pg_save.total_tries_used + (4 - ps.tries_remaining));
                pg_save.current_puzzle_index++;
                pg_save.current_puzzle_fails = 0;
                // Clear in-progress
                pg_save.ip_tries_remaining = 0;
                pg_save.ip_groups_solved   = 0;
                pg_save.ip_selected_mask   = 0;
                pg_save.ip_elapsed_seconds = 0;
                save_store(&pg_save);

                *next_scene = SCENE_WIN;
            }
        } else {
            // Wrong group
            ps.tries_remaining--;
            uint8_t data[8] = {
                (uint8_t)(ps.selected_mask & 0xFF),
                (uint8_t)((ps.selected_mask >> 8) & 0xFF)
            };
            anim_start(ANIM_CELL_FLASH, data, 12);
            sfx_wrong();
            // Only the TRIES count in the header changes — repaint
            // just the header line (incremental, same anti-flicker
            // pattern as A/B). Cells remain selected for the CELL_FLASH
            // visual so they don't need repainting.
            render_header();

            save_in_progress();

            if (ps.tries_remaining == 0) {
                pg_save.current_puzzle_fails++;
                pg_save.ip_tries_remaining = 0;
                pg_save.ip_groups_solved   = 0;
                pg_save.ip_selected_mask   = 0;
                pg_save.ip_elapsed_seconds = 0;
                save_store(&pg_save);
                *next_scene = SCENE_LOSE;
            }
        }
    }
}

static void play_teardown(void) {
    move_sprite(CURSOR_SPRITE_INDEX, 0, 0);
}

const SceneVTable SCENE_PLAY_VTABLE = {
    .init = play_init,
    .update = play_update,
    .render = play_render,
    .teardown = play_teardown,
};
