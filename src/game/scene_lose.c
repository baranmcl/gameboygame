#include "scene.h"
#include "../engine/render.h"
#include "../engine/input.h"

static void lose_init(void) {
    render_clear();
    render_text(2, 4, "SCENE LOSE");
    render_text(2, 8, "START -> ALL DONE");
}

static void lose_update(Scene *next_scene) {
    if (input_pressed(BTN_START)) *next_scene = SCENE_ALL_DONE;
}

static void lose_render(void) {
}

static void lose_teardown(void) {
}

const SceneVTable SCENE_LOSE_VTABLE = {
    .init = lose_init,
    .update = lose_update,
    .render = lose_render,
    .teardown = lose_teardown,
};
