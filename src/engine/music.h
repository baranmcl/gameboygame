#ifndef ENGINE_MUSIC_H
#define ENGINE_MUSIC_H

#include <stdint.h>
#include <stdbool.h>

// Music note indices. 0..N-1 reference a shared frequency table in
// music.c; 0xFF is a special "rest" marker that silences CH1.
//
// We support a small set of notes covering 2 octaves (C4..C6) — enough
// for simple chiptune melodies without bloating the frequency table.
// Each note maps to a (NR13, NR14) pair in music_note_table[] (music.c).
#define MUSIC_NOTE_C4   0
#define MUSIC_NOTE_D4   1
#define MUSIC_NOTE_E4   2
#define MUSIC_NOTE_F4   3
#define MUSIC_NOTE_G4   4
#define MUSIC_NOTE_A4   5
#define MUSIC_NOTE_B4   6
#define MUSIC_NOTE_C5   7
#define MUSIC_NOTE_D5   8
#define MUSIC_NOTE_E5   9
#define MUSIC_NOTE_F5  10
#define MUSIC_NOTE_G5  11
#define MUSIC_NOTE_A5  12
#define MUSIC_NOTE_B5  13
#define MUSIC_NOTE_C6  14
#define MUSIC_NOTE_REST 0xFF

#define MUSIC_NOTE_COUNT 15

// v1.2: drum types for the CH4 noise channel. CH4 has no concept of
// "note pitch" — instead these enum values map to preset NR42 (envelope)
// + NR43 (noise control) values in music.c. Use MUSIC_DRUM_REST to
// leave CH4 silent for a given step.
#define MUSIC_DRUM_REST  0
#define MUSIC_DRUM_KICK  1
#define MUSIC_DRUM_SNARE 2

#define MUSIC_DRUM_COUNT 3

// One step of a track: play `note` for `duration_frames` frames (60 Hz),
// then advance to the next step. note == MUSIC_NOTE_REST silences CH1
// for the duration.
typedef struct {
    uint8_t  note;
    uint8_t  duration_frames;
} MusicStep;

// A complete track: array of steps with a loop point. Playhead jumps
// from step `length - 1` back to step `loop_start` after that step's
// duration elapses. For non-looping tracks set loop_start == length
// (player will stop after the last step).
//
// v1.2: ch2_notes is an optional parallel array providing a CH2 harmony
// voice. NULL = CH1-only (existing tracks unchanged). If non-NULL, MUST
// be `length` elements long; CH2 note transitions happen at the same
// frame boundaries as CH1 (lockstep). Each entry is a MUSIC_NOTE_*
// value (or MUSIC_NOTE_REST to silence CH2 for that step).
//
// v1.2: ch4_drums is an optional parallel array providing a CH4 (noise)
// drum voice. Same lockstep behavior — drum hits trigger at CH1 step
// boundaries. Each entry is a MUSIC_DRUM_* value (KICK/SNARE/REST).
typedef struct {
    const MusicStep *steps;
    const uint8_t   *ch2_notes;
    const uint8_t   *ch4_drums;
    uint8_t          length;
    uint8_t          loop_start;
} MusicTrack;

// Initialize the music subsystem. Currently a no-op — provided for
// API symmetry with sound_init / render_init. Safe to call multiple
// times.
void music_init(void);

// Start playing `track`. Resets playhead to step 0. If a track was
// already playing, this overrides it. Pass NULL to stop (or use
// music_stop()).
void music_play(const MusicTrack *track);

// Stop the current track and silence CH1. Idempotent.
void music_stop(void);

// Per-frame tick. MUST be called from the VBlank ISR exactly once per
// frame (60 Hz). Advances the playhead and writes NR13/NR14 if a new
// note starts on this frame.
void music_tick(void);

// Returns true if a track is currently playing (i.e., not stopped /
// not at end of non-looping track).
bool music_is_playing(void);

// --- Testability hooks ---
//
// Host-side tests can query these to verify state machine behavior
// without needing to mock GB hardware registers. NOT for production
// use from scenes.

// Index of the step currently playing (0..track->length - 1), or 0xFF
// if no track is playing.
uint8_t music_current_step(void);

// Frames remaining in the current step before advancement. Useful for
// tests of looping and step-boundary edge cases.
uint8_t music_step_frames_remaining(void);

#endif
