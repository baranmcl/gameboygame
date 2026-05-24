#ifndef ENGINE_SOUND_H
#define ENGINE_SOUND_H

// Initialize the sound subsystem (enable all channels at boot).
void sound_init(void);

// Multi-step SFX driver: call once per frame from VBlank handler to advance
// any in-progress multi-note SFX (correct, win, lose, etc.).
void sound_tick(void);

// SFX triggers — fire-and-forget. Latest call interrupts any in-progress SFX.
void sfx_move(void);       // cursor moves
void sfx_select(void);     // A toggled selection ON
void sfx_deselect(void);   // A toggled selection OFF
void sfx_reject(void);     // A on full selection / START with <4 selected
void sfx_correct(void);    // submit → correct group
void sfx_wrong(void);      // submit → wrong group
void sfx_win(void);        // PLAY → WIN
void sfx_lose(void);       // PLAY → LOSE
void sfx_skip(void);       // player chose SKIP

#endif
