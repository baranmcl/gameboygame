#include <gb/gb.h>
#include <stdio.h>
#include "engine/render.h"
#include "engine/input.h"
#include "engine/save.h"

static char buf[21];

static void show_save_state(const GameSave *s, bool was_valid) {
    render_clear();
    render_text(2, 1, "SAVE SMOKE TEST");
    render_text(2, 3, was_valid ? "LOAD: VALID" : "LOAD: RESET");

    sprintf(buf, "IDX:%d FAILS:%d", s->current_puzzle_index, s->current_puzzle_fails);
    render_text(2, 5, buf);

    sprintf(buf, "SOLVED:%d SKIP:%d", s->puzzles_solved_total, s->puzzles_skipped_total);
    render_text(2, 6, buf);

    sprintf(buf, "STREAK:%d BEST:%d", s->current_streak, s->best_streak);
    render_text(2, 7, buf);

    render_text(2, 10, "A: BUMP SOLVED");
    render_text(2, 11, "B: RESET SAVE");
}

void main(void) {
    BGP_REG = 0xE4;
    render_init();

    GameSave save;
    bool was_valid = save_load(&save);

    show_save_state(&save, was_valid);

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        input_update();

        if (input_pressed(BTN_A)) {
            save.puzzles_solved_total++;
            save_store(&save);
            show_save_state(&save, was_valid);
        }
        if (input_pressed(BTN_B)) {
            save_reset(&save);
            save_store(&save);
            was_valid = false;
            show_save_state(&save, was_valid);
        }

        wait_vbl_done();
        render_flush();
    }
}
