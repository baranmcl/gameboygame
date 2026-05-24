#include <gb/gb.h>
#include <stdio.h>
#include "engine/render.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/sound.h"
#include "engine/anim.h"
#include "game/puzzles_types.h"

void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();

    char buf[21];

    render_text(2, 1, "PLAN A COMPLETE");

    sprintf(buf, "NUM_PUZZLES: %d", NUM_PUZZLES);
    render_text(2, 3, buf);

    sprintf(buf, "P1 W0: %s", PUZZLES[0].words[0]);
    render_text(2, 5, buf);
    sprintf(buf, "P1 W1: %s", PUZZLES[0].words[1]);
    render_text(2, 6, buf);
    sprintf(buf, "P1 GRP[0]: %d", PUZZLES[0].group_of[0]);
    render_text(2, 8, buf);
    sprintf(buf, "CAT 0: %s", PUZZLES[0].category_names[0]);
    render_text(2, 10, buf);

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        wait_vbl_done();
        render_flush();
    }
}
