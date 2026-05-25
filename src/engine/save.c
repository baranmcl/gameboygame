#include "save.h"
#include <gb/gb.h>
#include <stddef.h>
#include <string.h>

// Compile-time guards: GameSave's struct layout MUST match the SRAM
// byte layout documented in game_state.h. If SDCC adds padding for any
// uint16_t field, sizeof or offsets will diverge — silently corrupting
// every save/load.
//
// These typedef arrays trigger a compile error ("negative array size")
// if the assertion fails.
typedef char _check_GameSave_size[sizeof(GameSave) == 20 ? 1 : -1];
typedef char _check_cpi_offset[offsetof(GameSave, current_puzzle_index) == 5 ? 1 : -1];
typedef char _check_ip_tries_offset[offsetof(GameSave, ip_tries_remaining) == 13 ? 1 : -1];
typedef char _check_checksum_offset[offsetof(GameSave, checksum) == 19 ? 1 : -1];

static uint8_t compute_checksum(const GameSave *s) {
    const uint8_t *p = (const uint8_t *)s;
    uint8_t x = 0;
    for (uint8_t i = 0; i < 19; i++) x ^= p[i];
    return x;
}

void save_reset(GameSave *out) {
    memset(out, 0, sizeof(GameSave));
    out->magic[0] = SAVE_MAGIC_0;
    out->magic[1] = SAVE_MAGIC_1;
    out->magic[2] = SAVE_MAGIC_2;
    out->magic[3] = SAVE_MAGIC_3;
    out->version  = SAVE_VERSION;
    out->checksum = compute_checksum(out);
}

bool save_store(const GameSave *in) {
    GameSave tmp = *in;
    tmp.checksum = compute_checksum(&tmp);
    ENABLE_RAM_MBC1;
    SWITCH_RAM_MBC1(0);
    memcpy((void *)0xA000, &tmp, sizeof(GameSave));
    DISABLE_RAM_MBC1;
    return true;
}

bool save_load(GameSave *out) {
    GameSave tmp;
    ENABLE_RAM_MBC1;
    SWITCH_RAM_MBC1(0);
    memcpy(&tmp, (void *)0xA000, sizeof(GameSave));
    DISABLE_RAM_MBC1;

    if (tmp.magic[0] != SAVE_MAGIC_0 || tmp.magic[1] != SAVE_MAGIC_1
     || tmp.magic[2] != SAVE_MAGIC_2 || tmp.magic[3] != SAVE_MAGIC_3) {
        save_reset(out);
        save_store(out);
        return false;
    }

    if (tmp.version != SAVE_VERSION) {
        save_reset(out);
        save_store(out);
        return false;
    }

    if (compute_checksum(&tmp) != tmp.checksum) {
        save_reset(out);
        save_store(out);
        return false;
    }

    // Sanity checks: any field obviously out of range = corruption,
    // reset to defaults. Hardcoded upper bounds chosen as "way larger
    // than any legitimate value" — defense in depth against bit rot,
    // wild pointer writes, off-by-one over-reads, etc.
    //
    // We use 100 as a generous upper bound rather than NUM_PUZZLES (5)
    // for current_puzzle_index because (a) NUM_PUZZLES changes when the
    // bank grows, and any-value-over-100 is clearly garbage regardless;
    // (b) using NUM_PUZZLES has shown runtime read issues we haven't
    // fully diagnosed (see Discoveries — extern const from another TU
    // doesn't reliably resolve to the expected value at runtime under
    // SDCC for some reason).
    if (tmp.ip_groups_solved > 0x0F
     || tmp.current_puzzle_index > 100
     || tmp.current_puzzle_fails > 100
     || tmp.ip_tries_remaining > 4) {
        save_reset(out);
        save_store(out);
        return false;
    }

    *out = tmp;
    return true;
}
