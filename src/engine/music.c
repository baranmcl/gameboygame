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

// v1.2: emit a drum hit on CH4 (noise channel). CH4 has no concept of
// "note pitch" — each drum type maps to a fixed (envelope, noise-control)
// pair tuned by ear (v1.2.5b adjustments per user feedback: kick volume
// down + decay slowed for smoother feel; snare decay sped up for shorter
// sustain).
//
//   KICK: lower-frequency noise with gradual decay → "soft thump"
//         NR42 = 0x53 (vol 5, decay, step 3 — was vol 8 step 2)
//         NR43 = 0x64 (shift clock 6, 15-bit smooth pattern, divisor 4)
//
//   SNARE: mid-frequency noise, snap-and-end → "tight snap"
//         NR42 = 0x51 (vol 5, decay, step 1 — was step 3)
//         NR43 = 0x33 (shift clock 3, 15-bit, divisor 3)
//
//   REST: silence CH4 by zeroing envelope
//
// On host (gcc) the body is compiled out — register macros aren't
// available; state machine timing is what we want host-tested.
static void emit_drum(uint8_t drum) {
#ifdef __SDCC
    if (drum == MUSIC_DRUM_REST) {
        NR42_REG = 0x00;
        NR44_REG = 0x80;
        return;
    }
    if (drum == MUSIC_DRUM_KICK) {
        NR41_REG = 0x00;
        NR42_REG = 0x53;
        NR43_REG = 0x64;
        NR44_REG = 0x80;
        return;
    }
    if (drum == MUSIC_DRUM_SNARE) {
        NR41_REG = 0x00;
        NR42_REG = 0x51;
        NR43_REG = 0x33;
        NR44_REG = 0x80;
        return;
    }
    // Unknown drum type — silence (defensive).
    NR42_REG = 0x00;
    NR44_REG = 0x80;
#else
    (void)drum;
#endif
}

// v1.2: emit a CH2 harmony note. Mirrors emit_note but uses NR2x
// registers (CH2 has no frequency sweep — no equivalent of NR10).
// Volume one tier lower than CH1 (5 vs 6) so the harmony sits below
// the melody rather than competing with it.
static void emit_note_ch2(uint8_t note) {
#ifdef __SDCC
    if (note == MUSIC_NOTE_REST) {
        NR22_REG = 0x00;
        NR24_REG = 0x80;
        return;
    }
    if (note >= MUSIC_NOTE_COUNT) return;
    NR21_REG = 0x40;        // 25% duty cycle, matches CH1
    NR22_REG = 0x53;        // volume 5 (one below CH1's 6), decay, step 3
    NR23_REG = music_note_table[note].lo;
    NR24_REG = music_note_table[note].hi;
#else
    (void)note;
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
    // v1.2: emit the lockstep CH2 note if the track has a harmony layer.
    if (track->ch2_notes) {
        emit_note_ch2(track->ch2_notes[0]);
    }
    // v1.2: emit the lockstep CH4 drum if the track has a drum layer.
    if (track->ch4_drums) {
        emit_drum(track->ch4_drums[0]);
    }
}

void music_stop(void) {
    active_track = 0;
    current_step_idx = 0xFF;
    frames_remaining = 0;
    emit_note(MUSIC_NOTE_REST);
    emit_note_ch2(MUSIC_NOTE_REST);  // also silence CH2 (no-op if never used)
    emit_drum(MUSIC_DRUM_REST);      // also silence CH4 (no-op if never used)
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
    // v1.2: lockstep CH2 advance — same step boundary, same frame.
    if (active_track->ch2_notes) {
        emit_note_ch2(active_track->ch2_notes[next_idx]);
    }
    // v1.2: lockstep CH4 drum advance — same step boundary.
    if (active_track->ch4_drums) {
        emit_drum(active_track->ch4_drums[next_idx]);
    }
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
