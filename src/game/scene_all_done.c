#include "scene.h"
#include "../engine/render.h"
#include "../engine/input.h"

static void done_init(void) {
    render_clear();
    render_text(2, 4, "SCENE ALL DONE");
    render_text(2, 8, "START -> TITLE");
}

static void done_update(Scene *next_scene) {
    if (input_pressed(BTN_START)) *next_scene = SCENE_TITLE;
}

static void done_render(void) {
}

static void done_teardown(void) {
}

const SceneVTable SCENE_ALL_DONE_VTABLE = {
    .init = done_init,
    .update = done_update,
    .render = done_render,
    .teardown = done_teardown,
};
