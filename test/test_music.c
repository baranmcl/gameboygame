// Host-side tests for the music state machine (src/engine/music.c).
// Builds with mingw gcc — uses no GBDK headers (those are #ifdef __SDCC
// guarded in music.c for this purpose). Tests the state machine's frame
// counting, step advancement, and loop behavior; does NOT test the
// emit_note hardware writes (untestable without a GB emulator).

#include "../src/engine/music.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT(cond) do { \
    tests_run++; \
    if (!(cond)) { \
        tests_failed++; \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

// ----- Test 1: empty player state after init -----
static void test_init_no_active_track(void) {
    music_init();
    ASSERT(music_is_playing() == 0);
    ASSERT(music_current_step() == 0xFF);
}

// ----- Test 2: music_play with NULL stops -----
static void test_play_null_stops(void) {
    music_init();
    music_play(0);
    ASSERT(music_is_playing() == 0);
}

// ----- Test 3: music_play with zero-length track stops -----
static void test_play_empty_track_stops(void) {
    music_init();
    MusicTrack t = { .steps = 0, .length = 0, .loop_start = 0 };
    music_play(&t);
    ASSERT(music_is_playing() == 0);
}

// ----- Test 4: single-step track advances frame counter then stops -----
static void test_single_step_no_loop_stops(void) {
    music_init();
    MusicStep steps[] = { { MUSIC_NOTE_C5, 3 } };
    MusicTrack t = { .steps = steps, .length = 1, .loop_start = 1 };  // no loop
    music_play(&t);
    ASSERT(music_is_playing() == 1);
    ASSERT(music_current_step() == 0);
    ASSERT(music_step_frames_remaining() == 3);

    music_tick();  // frames_remaining: 3 -> 2
    ASSERT(music_step_frames_remaining() == 2);

    music_tick();  // 2 -> 1
    ASSERT(music_step_frames_remaining() == 1);

    music_tick();  // 1 -> 0
    ASSERT(music_step_frames_remaining() == 0);

    music_tick();  // step ends, next_idx == length, loop_start >= length → stop
    ASSERT(music_is_playing() == 0);
}

// ----- Test 5: multi-step track advances through steps -----
static void test_multi_step_advancement(void) {
    music_init();
    MusicStep steps[] = {
        { MUSIC_NOTE_C5, 2 },
        { MUSIC_NOTE_E5, 2 },
        { MUSIC_NOTE_G5, 2 },
    };
    MusicTrack t = { .steps = steps, .length = 3, .loop_start = 3 };  // no loop
    music_play(&t);

    ASSERT(music_current_step() == 0);
    music_tick(); music_tick();  // step 0 done
    music_tick();  // advances to step 1
    ASSERT(music_current_step() == 1);
    ASSERT(music_step_frames_remaining() == 2);
    music_tick(); music_tick();  // step 1 done
    music_tick();  // advances to step 2
    ASSERT(music_current_step() == 2);
    music_tick(); music_tick();
    music_tick();  // end of track, no loop
    ASSERT(music_is_playing() == 0);
}

// ----- Test 6: looping track jumps back to loop_start -----
// Note: duration_frames=N means N frames audible THEN advance on the
// (N+1)th tick. So with duration=1, it takes 2 ticks per step
// transition: tick 1 decrements frames to 0, tick 2 advances.
static void test_loop_back(void) {
    music_init();
    MusicStep steps[] = {
        { MUSIC_NOTE_C5, 1 },  // step 0 — only played once (intro)
        { MUSIC_NOTE_D5, 1 },  // step 1 — looped
        { MUSIC_NOTE_E5, 1 },  // step 2 — looped
    };
    MusicTrack t = { .steps = steps, .length = 3, .loop_start = 1 };
    music_play(&t);

    ASSERT(music_current_step() == 0);
    music_tick(); music_tick();  // step 0 done, advances to step 1
    ASSERT(music_current_step() == 1);
    music_tick(); music_tick();  // step 1 done, advances to step 2
    ASSERT(music_current_step() == 2);
    music_tick(); music_tick();  // step 2 done, loop back to loop_start = 1
    ASSERT(music_current_step() == 1);
    ASSERT(music_is_playing() == 1);  // still playing after loop
    music_tick(); music_tick();  // step 1 done again, advances to step 2
    ASSERT(music_current_step() == 2);
}

// ----- Test 7: music_stop while playing silences the channel -----
static void test_stop_while_playing(void) {
    music_init();
    MusicStep steps[] = { { MUSIC_NOTE_C5, 10 } };
    MusicTrack t = { .steps = steps, .length = 1, .loop_start = 1 };
    music_play(&t);
    ASSERT(music_is_playing() == 1);
    music_stop();
    ASSERT(music_is_playing() == 0);
    ASSERT(music_current_step() == 0xFF);
}

// ----- Test 8: REST note advances normally -----
static void test_rest_note_advances(void) {
    music_init();
    MusicStep steps[] = {
        { MUSIC_NOTE_C5,  2 },
        { MUSIC_NOTE_REST, 2 },
        { MUSIC_NOTE_E5,  2 },
    };
    MusicTrack t = { .steps = steps, .length = 3, .loop_start = 3 };
    music_play(&t);

    music_tick(); music_tick();  // step 0 done
    music_tick();  // step 1 (REST)
    ASSERT(music_current_step() == 1);
    music_tick(); music_tick();  // step 1 done
    music_tick();  // step 2
    ASSERT(music_current_step() == 2);
}

int main(void) {
    test_init_no_active_track();
    test_play_null_stops();
    test_play_empty_track_stops();
    test_single_step_no_loop_stops();
    test_multi_step_advancement();
    test_loop_back();
    test_stop_while_playing();
    test_rest_note_advances();

    printf("Tests run: %d, Failed: %d\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
