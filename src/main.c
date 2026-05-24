#include <gb/gb.h>
#include "engine/render.h"
#include "engine/input.h"

void main(void) {
    BGP_REG = 0xE4;
    render_init();

    SHOW_BKG;
    DISPLAY_ON;

    char line[21];
    uint8_t press_flash = 0;

    while (1) {
        input_update();

        line[0] = input_held(BTN_UP)     ? 'U' : '-';
        line[1] = input_held(BTN_DOWN)   ? 'D' : '-';
        line[2] = input_held(BTN_LEFT)   ? 'L' : '-';
        line[3] = input_held(BTN_RIGHT)  ? 'R' : '-';
        line[4] = ' ';
        line[5] = input_held(BTN_A)      ? 'A' : '-';
        line[6] = input_held(BTN_B)      ? 'B' : '-';
        line[7] = ' ';
        line[8] = input_held(BTN_START)  ? 'T' : '-';
        line[9] = input_held(BTN_SELECT) ? 'S' : '-';
        line[10] = 0;

        render_clear();
        render_text(2, 2, "INPUT SMOKE TEST");
        render_text(2, 4, "HELD:");
        render_text(2, 5, line);

        if (input_pressed(BTN_A)) press_flash = 30;
        if (press_flash > 0) {
            render_text(2, 7, "A PRESSED!");
            press_flash--;
        }

        wait_vbl_done();
        render_flush();
    }
}
