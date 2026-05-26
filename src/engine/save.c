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
// if the assertion fails. v2 layout: 224 bytes total, v1 fields preserved
// at offsets [0..19] for migration compatibility.
typedef char _check_GameSave_size[sizeof(GameSave) == 224 ? 1 : -1];
typedef char _check_cpi_offset[offsetof(GameSave, current_puzzle_index) == 5 ? 1 : -1];
typedef char _check_ip_tries_offset[offsetof(GameSave, ip_tries_remaining) == 13 ? 1 : -1];
typedef char _check_v1_checksum_offset[offsetof(GameSave, v1_checksum) == 19 ? 1 : -1];
typedef char _check_completed_offset[offsetof(GameSave, completed_bits) == 20 ? 1 : -1];
typedef char _check_best_time_offset[offsetof(GameSave, puzzle_best_time) == 28 ? 1 : -1];
typedef char _check_best_tries_offset[offsetof(GameSave, puzzle_best_tries) == 156 ? 1 : -1];
typedef char _check_v2_checksum_offset[offsetof(GameSave, checksum) == 223 ? 1 : -1];

// Legacy v1 (v1.0/v1.1) save layout — 20 bytes. Used ONLY for reading
// existing player saves during migration. Field order MUST match the
// original v1 struct exactly (it's the byte layout that's actually in
// SRAM on a v1.1-installed cart).
typedef struct {
    uint8_t  magic[4];
    uint8_t  version;
    uint8_t  current_puzzle_index;
    uint8_t  current_puzzle_fails;
    uint8_t  puzzles_solved_total;
    uint8_t  puzzles_skipped_total;
    uint8_t  current_streak;
    uint8_t  best_streak;
    uint16_t total_tries_used;
    uint8_t  ip_tries_remaining;
    uint8_t  ip_groups_solved;
    uint16_t ip_selected_mask;
    uint16_t ip_elapsed_seconds;
    uint8_t  checksum;
} LegacyGameSaveV1;

typedef char _check_LegacyV1_size[sizeof(LegacyGameSaveV1) == 20 ? 1 : -1];

// v1 checksum: XOR of bytes [0..18]. Different from v2's checksum
// (which spans [0..222]).
static uint8_t compute_v1_checksum(const LegacyGameSaveV1 *s) {
    const uint8_t *p = (const uint8_t *)s;
    uint8_t x = 0;
    for (uint8_t i = 0; i < 19; i++) x ^= p[i];
    return x;
}

static uint8_t compute_checksum(const GameSave *s) {
    // v2 checksum covers bytes [0..222] — everything before the checksum
    // byte at offset 223. Note that byte [19] (v1_checksum slot) is now
    // just data; we cover it too.
    const uint8_t *p = (const uint8_t *)s;
    uint8_t x = 0;
    for (uint16_t i = 0; i < 223; i++) x ^= p[i];
    return x;
}

// Migrate a validated v1 save to a v2 GameSave in `out`. The completed
// bitmap is backfilled from current_puzzle_index — linear progression
// guarantees puzzles 0..N-1 were completed to reach puzzle N. Per-puzzle
// records are zeroed (no historical data — players who replay see no
// prior record until they set a new one).
static void migrate_v1_to_v2(const LegacyGameSaveV1 *legacy, GameSave *out) {
    memset(out, 0, sizeof(GameSave));
    out->magic[0]               = SAVE_MAGIC_0;
    out->magic[1]               = SAVE_MAGIC_1;
    out->magic[2]               = SAVE_MAGIC_2;
    out->magic[3]               = SAVE_MAGIC_3;
    out->version                = SAVE_VERSION;  // 2
    out->current_puzzle_index   = legacy->current_puzzle_index;
    out->current_puzzle_fails   = legacy->current_puzzle_fails;
    out->puzzles_solved_total   = legacy->puzzles_solved_total;
    out->puzzles_skipped_total  = legacy->puzzles_skipped_total;
    out->current_streak         = legacy->current_streak;
    out->best_streak            = legacy->best_streak;
    out->total_tries_used       = legacy->total_tries_used;
    out->ip_tries_remaining     = legacy->ip_tries_remaining;
    out->ip_groups_solved       = legacy->ip_groups_solved;
    out->ip_selected_mask       = legacy->ip_selected_mask;
    out->ip_elapsed_seconds     = legacy->ip_elapsed_seconds;
    out->v1_checksum            = legacy->checksum;  // preserve legacy byte at offset 19

    // Backfill completed_bits: mark puzzles 0..(current_puzzle_index-1)
    // complete. Linear progression invariant — to reach puzzle N you
    // must have completed 0..N-1. Cap at MAX_PUZZLES_SUPPORTED in case
    // a legacy save somehow has current_puzzle_index > 64.
    uint8_t backfill_count = legacy->current_puzzle_index;
    if (backfill_count > MAX_PUZZLES_SUPPORTED) backfill_count = MAX_PUZZLES_SUPPORTED;
    for (uint8_t i = 0; i < backfill_count; i++) {
        out->completed_bits[i >> 3] |= (uint8_t)(1u << (i & 7));
    }
    // puzzle_best_time and puzzle_best_tries left as 0 (sentinel for "no record").

    out->checksum = compute_checksum(out);
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
    // First, read the 20-byte v1 header view. We can't memcpy
    // sizeof(GameSave) yet because we don't know if the SRAM contains
    // a v1 save (20 bytes valid) or a v2 save (224 bytes valid). The
    // legacy view tells us the version, then we re-read appropriately.
    LegacyGameSaveV1 legacy;
    ENABLE_RAM_MBC1;
    SWITCH_RAM_MBC1(0);
    memcpy(&legacy, (void *)0xA000, sizeof(LegacyGameSaveV1));

    // Magic check — same magic in all save versions.
    if (legacy.magic[0] != SAVE_MAGIC_0 || legacy.magic[1] != SAVE_MAGIC_1
     || legacy.magic[2] != SAVE_MAGIC_2 || legacy.magic[3] != SAVE_MAGIC_3) {
        DISABLE_RAM_MBC1;
        save_reset(out);
        save_store(out);
        return false;
    }

    if (legacy.version == 1) {
        // Legacy v1.0/v1.1 save — validate v1 checksum, then migrate.
        if (compute_v1_checksum(&legacy) != legacy.checksum) {
            DISABLE_RAM_MBC1;
            save_reset(out);
            save_store(out);
            return false;
        }
        DISABLE_RAM_MBC1;
        migrate_v1_to_v2(&legacy, out);
        // Immediately persist as v2 so the next save_load is the fast
        // v2 path (no double migration).
        save_store(out);
        return true;
    }

    if (legacy.version == SAVE_VERSION) {
        // v2 save — re-read the full struct now that we know the size.
        GameSave tmp;
        memcpy(&tmp, (void *)0xA000, sizeof(GameSave));
        DISABLE_RAM_MBC1;

        if (compute_checksum(&tmp) != tmp.checksum) {
            save_reset(out);
            save_store(out);
            return false;
        }

        // Sanity checks: any field obviously out of range = corruption,
        // reset to defaults. Same hardcoded bounds as v1 — see prior
        // discoveries note about NUM_PUZZLES extern resolution issues.
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

    // Unknown / future version — wipe and start fresh. When v3 lands,
    // add a `legacy.version == 2` branch above for v2→v3 migration.
    DISABLE_RAM_MBC1;
    save_reset(out);
    save_store(out);
    return false;
}
