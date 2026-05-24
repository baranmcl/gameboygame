#include "scene.h"
#include "../engine/render.h"
#include "../engine/input.h"

static void play_init(void) {
    render_clear();
    render_text(2, 4, "SCENE PLAY");
    render_text(2, 8, "START -> WIN");
}

static void play_update(Scene *next_scene) {
    if (input_pressed(BTN_START)) *next_scene = SCENE_WIN;
}

static void play_render(void) {
}

static void play_teardown(void) {
}

const SceneVTable SCENE_PLAY_VTABLE = {
    .init = play_init,
    .update = play_update,
    .render = play_render,
    .teardown = play_teardown,
};
