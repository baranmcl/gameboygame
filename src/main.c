#include <gb/gb.h>
#include "engine/render.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/sound.h"
#include "engine/anim.h"
#include "game/scene.h"
#include "game/game_state.h"

extern const SceneVTable SCENE_TITLE_VTABLE;
extern const SceneVTable SCENE_PLAY_VTABLE;
extern const SceneVTable SCENE_WIN_VTABLE;
extern const SceneVTable SCENE_LOSE_VTABLE;
extern const SceneVTable SCENE_ALL_DONE_VTABLE;

// Scene table — populated imperatively in main() (see workaround note).
SceneVTable SCENES[5];

// VBlank ISR — called by GBDK once per frame (60 Hz on DMG).
// Runs the per-frame engine-side ticks. Keep this lean: VBlank window
// is short (~1.1ms), and overrunning corrupts the next frame's rendering.
static void vblank_isr(void) {
    anim_tick();
    sound_tick();
    render_flush();
}

void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();

    // SDCC has shown inconsistent behavior with designated-initializer
    // arrays of structs whose members are function pointers defined in
    // other translation units — populate imperatively to sidestep any
    // initializer-time copy issues.
    SCENES[SCENE_TITLE]    = SCENE_TITLE_VTABLE;
    SCENES[SCENE_PLAY]     = SCENE_PLAY_VTABLE;
    SCENES[SCENE_WIN]      = SCENE_WIN_VTABLE;
    SCENES[SCENE_LOSE]     = SCENE_LOSE_VTABLE;
    SCENES[SCENE_ALL_DONE] = SCENE_ALL_DONE_VTABLE;

    add_VBL(vblank_isr);

    Scene current = SCENE_TITLE;
    SCENES[current].init();

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    while (1) {
        input_update();

        Scene next = current;
        SCENES[current].update(&next);
        SCENES[current].render();

        if (next != current) {
            SCENES[current].teardown();
            current = next;
            SCENES[current].init();
        }

        wait_vbl_done();
    }
}
