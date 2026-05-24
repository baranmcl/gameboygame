#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include "game_state.h"

// Each scene exposes 4 lifecycle hooks. main.c's dispatcher calls them
// in this order each frame:
//   1. If transitioning into this scene, call init() once.
//   2. update() — read input, mutate scene-internal state, schedule
//      animations, trigger transitions by setting *next_scene.
//   3. render() — write to the tilemap buffer (via engine/render).
//      MUST NOT touch VRAM directly; engine/render flushes during VBlank.
//   4. If transitioning out, call teardown() once.
//
// Scenes set *next_scene from inside update() to request a transition.
// next_scene == current_scene means "stay". The dispatcher in main.c
// reads *next_scene after update() runs.

typedef struct {
    void (*init)(void);
    void (*update)(Scene *next_scene);
    void (*render)(void);
    void (*teardown)(void);
} SceneVTable;

// Non-const because main.c populates it imperatively at boot (SDCC
// designated-initializer quirk with cross-TU function pointers).
extern SceneVTable SCENES[];

#endif
