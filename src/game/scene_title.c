#include "scene.h"
#include "../engine/render.h"
#include "../engine/input.h"

static void title_init(void) {
    render_clear();
    render_text(2, 4, "SCENE TITLE");
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
