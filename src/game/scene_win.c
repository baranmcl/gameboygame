#include "scene.h"
#include "../engine/render.h"
#include "../engine/input.h"

static void win_init(void) {
    render_clear();
    render_text(2, 4, "SCENE WIN");
    render_text(2, 8, "START -> LOSE");
}

static void win_update(Scene *next_scene) {
    if (input_pressed(BTN_START)) *next_scene = SCENE_LOSE;
}

static void win_render(void) {
}

static void win_teardown(void) {
}

const SceneVTable SCENE_WIN_VTABLE = {
    .init = win_init,
    .update = win_update,
    .render = win_render,
    .teardown = win_teardown,
};
