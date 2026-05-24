#ifndef ENGINE_SAVE_H
#define ENGINE_SAVE_H

#include <stdint.h>
#include <stdbool.h>
#include "../game/game_state.h"

// Read SRAM into *out. Returns true if a valid save existed.
// If false (invalid magic, wrong version, bad checksum, sanity failure),
// *out is overwritten with defaults AND those defaults are immediately
// written back so subsequent loads succeed.
bool save_load(GameSave *out);

// Write *in to SRAM. Computes a fresh checksum on a local copy so the
// caller's struct is not mutated. Brackets the write with ENABLE_RAM_MBC1 /
// DISABLE_RAM_MBC1. Returns true on success.
bool save_store(const GameSave *in);

// Overwrite *out with factory-default values (NEW GAME).
// Does NOT write to SRAM — caller is responsible for calling save_store(out).
void save_reset(GameSave *out);

#endif
