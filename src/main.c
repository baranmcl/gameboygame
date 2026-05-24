#include <gb/gb.h>
#include "engine/render.h"

void main(void) {
    BGP_REG = 0xE4;

    render_init();
    render_text(2, 2, "RENDER OK");
    render_text(2, 4, "PLAN A PHASE 2");
    render_flush();

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        wait_vbl_done();
    }
}
