#include "scene.h"
#include "../engine/render.h"
#include "../engine/input.h"

static void title_init(void) {
    render_clear();
    render_text(2, 1, "PHASE 3 TILE TEST");
    render_text(2, 3, "UI TILES (row 5):");

    // Display tiles 64..76 horizontally on row 5
    for (uint8_t i = 0; i < 13; i++) {
        render_set_tile(2 + i, 5, UI_TILE_BASE + i);
    }

    render_text(2, 8, "START -> PLAY");
}

static void title_update(Scene *next_scene) {
    if (input_pressed(BTN_START)) *next_scene = SCENE_PLAY;
}

static void title_render(void) {
    // Stub: init() draws everything once; no per-frame redraw needed.
}

static void title_teardown(void) {
    // Nothing to clean up in the stub.
}

const SceneVTable SCENE_TITLE_VTABLE = {
    .init = title_init,
    .update = title_update,
    .render = title_render,
    .teardown = title_teardown,
};
