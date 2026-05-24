#include <gb/gb.h>
#include "engine/render.h"
#include "engine/input.h"
#include "engine/sound.h"

void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();

    render_text(2, 1, "SOUND SMOKE TEST");
    render_text(2, 3, "UP: MOVE");
    render_text(2, 4, "DOWN: SELECT");
    render_text(2, 5, "LEFT: DESELECT");
    render_text(2, 6, "RIGHT: REJECT");
    render_text(2, 7, "A: CORRECT");
    render_text(2, 8, "B: WRONG");
    render_text(2, 9, "START: WIN");
    render_text(2, 10, "SELECT: LOSE");
    render_text(2, 12, "A+B: SKIP");

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        input_update();

        if (input_pressed(BTN_UP))    sfx_move();
        if (input_pressed(BTN_DOWN))  sfx_select();
        if (input_pressed(BTN_LEFT))  sfx_deselect();
        if (input_pressed(BTN_RIGHT)) sfx_reject();

        if (input_pressed(BTN_A) && input_held(BTN_B))      sfx_skip();
        else if (input_pressed(BTN_A))                      sfx_correct();
        if (input_pressed(BTN_B) && !input_pressed(BTN_A))  sfx_wrong();

        if (input_pressed(BTN_START))  sfx_win();
        if (input_pressed(BTN_SELECT)) sfx_lose();

        wait_vbl_done();
        sound_tick();
        render_flush();
    }
}
