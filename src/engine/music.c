#include "music.h"
#include <stdint.h>
#ifdef __SDCC
#include <gb/gb.h>
#include <gb/hardware.h>
#endif

// Frequency table for the 15 notes defined in music.h. Format matches
// sound.c's NOTE_X_LO / NOTE_X_HI: NR13 = low 8 bits of the GB frequency
// register; NR14 = trigger(0x80) | length-disabled(0) | top 3 bits of
// frequency. See sound.c for the formula derivation.
typedef struct { uint8_t lo, hi; } NotePair;

static const NotePair music_note_table[MUSIC_NOTE_COUNT] = {
    { 0x0B, 0x86 },  // C4
    { 0x42, 0x86 },  // D4
    { 0x72, 0x86 },  // E4
    { 0x8A, 0x86 },  // F4
    { 0xB2, 0x86 },  // G4
    { 0xD6, 0x86 },  // A4
    { 0xF7, 0x86 },  // B4
    { 0x06, 0x87 },  // C5
    { 0x21, 0x87 },  // D5
    { 0x39, 0x87 },  // E5
    { 0x44, 0x87 },  // F5
    { 0x59, 0x87 },  // G5
    { 0x6B, 0x87 },  // A5
    { 0x7B, 0x87 },  // B5
    { 0x83, 0x87 },  // C6
};

// Player state.
static const MusicTrack *active_track = 0;
static uint8_t  current_step_idx = 0xFF;
static uint8_t  frames_remaining = 0;

// Write NR10-NR14 to play the note (or silence on REST). On host (gcc)
// the body is compiled out — the GBDK register macros aren't available,
// and the state machine is what we want host-tested.
static void emit_note(uint8_t note) {
#ifdef __SDCC
    if (note == MUSIC_NOTE_REST) {
        // Silence CH1 by setting envelope to volume 0.
        NR12_REG = 0x00;
        // Trigger with the silence envelope so the channel stops emitting.
        NR14_REG = 0x80;
        return;
    }
    if (note >= MUSIC_NOTE_COUNT) return;  // defensive
    NR10_REG = 0x00;        // no frequency sweep
    NR11_REG = 0x40;        // 25% duty cycle — rounder tone vs 50%'s buzz
    NR12_REG = 0x63;        // volume 6 (down from 8), decay, envelope step 3
    NR13_REG = music_note_table[note].lo;
    NR14_REG = music_note_table[note].hi;
#else
    (void)note;
    (void)music_note_table;
#endif
}

void music_init(void) {
    active_track = 0;
    current_step_idx = 0xFF;
    frames_remaining = 0;
}

void music_play(const MusicTrack *track) {
    if (!track || track->length == 0) {
        music_stop();
        return;
    }
    active_track = track;
    current_step_idx = 0;
    frames_remaining = track->steps[0].duration_frames;
    emit_note(track->steps[0].note);
}

void music_stop(void) {
    active_track = 0;
    current_step_idx = 0xFF;
    frames_remaining = 0;
    emit_note(MUSIC_NOTE_REST);
}

void music_tick(void) {
    if (!active_track) return;
    if (frames_remaining > 0) {
        frames_remaining--;
        return;
    }
    // Advance to the next step.
    uint8_t next_idx = (uint8_t)(current_step_idx + 1);
    if (next_idx >= active_track->length) {
        // End of track — loop if loop_start is in-range, else stop.
        if (active_track->loop_start < active_track->length) {
            next_idx = active_track->loop_start;
        } else {
            music_stop();
            return;
        }
    }
    current_step_idx = next_idx;
    frames_remaining = active_track->steps[next_idx].duration_frames;
    emit_note(active_track->steps[next_idx].note);
}

bool music_is_playing(void) {
    return active_track != 0;
}

uint8_t music_current_step(void) {
    return current_step_idx;
}

uint8_t music_step_frames_remaining(void) {
    return frames_remaining;
}
