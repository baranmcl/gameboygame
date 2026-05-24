#include <gb/gb.h>
#include "engine/render.h"
#include "engine/input.h"
#include "engine/sound.h"
#include "engine/anim.h"

void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();

    render_text(2, 1, "ANIM SMOKE TEST");
    render_text(2, 3, "A: WRONG SHAKE");
    render_text(2, 4, "(SCREEN JITTERS");
    render_text(2, 5, " FOR 0.1S)");
    render_text(2, 7, "B: SELECT FLASH");
    render_text(2, 8, "(STUB: GATES INPUT");
    render_text(2, 9, " FOR 4 FRAMES)");

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        input_update();

        if (!anim_is_playing()) {
            if (input_pressed(BTN_A)) {
                anim_start(ANIM_WRONG_SHAKE, 0, 6);
                sfx_wrong();
            }
            if (input_pressed(BTN_B)) {
                anim_start(ANIM_SELECT_FLASH, 0, 4);
                sfx_select();
            }
        }

        wait_vbl_done();
        anim_tick();
        sound_tick();
        render_flush();
    }
}
