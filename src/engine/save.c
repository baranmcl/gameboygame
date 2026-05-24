#include "save.h"
#include <gb/gb.h>
#include <string.h>

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

    // ip_groups_solved must fit in 4 bits (one bit per group, 4 groups).
    // current_puzzle_index sanity check deferred to Phase 8 when NUM_PUZZLES exists.
    if (tmp.ip_groups_solved > 0x0F) {
        save_reset(out);
        save_store(out);
        return false;
    }

    *out = tmp;
    return true;
}
