# Replay & Polish (v1.2) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add puzzle-select replay + stats history + tiered milestone celebrations + audio harmony layer + small visual polish, bringing the v1.0/v1.1 game from "play 30 puzzles linearly" to "play, replay, track your best, see your progress."

**Architecture:** Save struct gains `completed_bits[8]` (64-bit bitmap, supports up to 64 puzzles) + per-puzzle `best_time` (uint16) and `best_tries` (uint8) records (~200 bytes total in SRAM). Migration path handles v1 → v2 saves by backfilling `completed_bits` from the existing `current_puzzle_index` (puzzles 0..N-1 inferred complete). New `SCENE_PUZZLE_SELECT` scene reads completion grid + lets player launch any completed puzzle. Music engine gains optional `ch2_notes` parallel array on `MusicTrack` so existing CH1-only tracks keep working unchanged. Two new milestone-celebration WIN-scene variants (one for every 5 lifetime solves, one for every 5 streak) fire based on save state.

**Tech Stack:** GBDK-2020 + SDCC, existing engine subsystems (render, save, sound, music, anim). Plan A's Python build_puzzles.py codegen pipeline handles the 50-puzzle bank automatically.

---

## Living Document Contract

This plan is a living document. Every executing agent MUST update it as
execution progresses, not only at completion.

- **On phase claim:** the executor MUST flip the banner to 🚧 IN PROGRESS
  with a claim timestamp (ISO 8601 UTC) and the active branch name. The
  banner MUST NOT include an expected-completion estimate — agents cannot
  reliably estimate their own wall-clock, and a fabricated duration
  becomes a stale anchor that misleads future readers. Followers
  encountering a 🚧 banner determine liveness by observable signals (PR
  existence, recent branch commits), not by arithmetic on expected times.
  See Step 5's stale-claim reclaim protocol.
- **On phase ship:** the executor MUST update that phase's **Execution
  Status** banner with the shipped commit SHA(s) and date. If a PR is
  open, the PR number and URL MUST appear in the top-of-plan Execution
  Status table.
- **On phase defer:** the executor MUST update the banner with ⏸ status
  AND a prose description of the unblock condition + a link to the
  likely-unblocker artifact (plan page, task, or PR whose own Execution
  Status banner will signal completion). Prose + link is durable across
  paraphrases and scope edits; exact-string coordination between agents
  is not.
- **On PR merge:** the executor MUST record the merge SHA in the banner
  + the top-of-plan Execution Status table.
- **On deviation from the written plan** (scope edits, structural
  refactors, dropped tasks, reordered phases): the executor MUST
  inline-document the deviation in the affected task AND summarize it
  in the top-of-plan Execution Status as a "Deviations" subsection.
  Deviation state MUST NOT live only in PR notes or status reports.
- **On discovery** (pre-existing drift surfaced during execution, new
  bugs found, architectural issues noted): the executor MUST add a
  "Discoveries" subsection at the top of the plan with pointers to the
  files/lines affected. Follow-up dispatches read this subsection to
  avoid duplicate discovery work.

The plan SHOULD reflect reality at the end of every session that touches
it. Anything worth putting in a status report to the user is worth
putting in the plan.

Rationale: `/writing-plans-enhanced` Step 5. Writing at ship time is
cheap; reconstruction by downstream readers is expensive, compounds
across dispatches, and fails silently when state is split across PR
notes and commit messages.

---

## Execution Status

**Overall:** Not started. 9 phases planned.

| Phase | Status | Ship SHA(s) | Notes |
|---|---|---|---|
| 1 — Save struct v2 migration + per-puzzle stats capture | ⬜ Not started | — | extend GameSave with completed_bits + best records; v1→v2 migration backfills |
| 2 — Cascade-bar slide-in (scene_win + scene_lose) | ⬜ Not started | — | SCX-based slide for bar reveal animations |
| 3 — Tiered milestone celebrations (lifetime + streak) | ⬜ Not started | — | small fanfare every 5 lifetime, bigger every 5 streak |
| 4 — Music engine: CH2 harmony support | ⬜ Not started | — | optional `ch2_notes` parallel array on MusicTrack; host tests |
| 5 — Title theme: CH2 harmony layer | ⬜ Not started | — | author harmony for title_theme.c |
| 6 — Puzzle-start CH1 stinger | ⬜ Not started | — | 4-note fanfare when PLAY scene loads |
| 7 — Puzzle-select scene + replay flow | ⬜ Not started | — | new SCENE_PUZZLE_SELECT; grid view; replay any completed puzzle |
| 8 — Smaller ALL_DONE polish | ⬜ Not started | — | longer chiptune flourish + scrolling text |
| 9 — Content: +20 puzzles (to 50 total) + v1.2 release | ⬜ Not started | — | DMG/GBC verify, README, tag, asset upload |

---

## Architecture Notes (read before any phase)

### Save struct migration philosophy

v1.0/v1.1 saves use `SAVE_VERSION = 1` with a fixed 20-byte struct. v1.2's struct grows by ~200 bytes (`completed_bits[8]` + `puzzle_best_time[64]` + `puzzle_best_tries[64]`). On encountering a v1 save, the executor MUST migrate, NOT wipe:
- Read first 20 bytes as `LegacyGameSaveV1` to validate the legacy magic + version + checksum.
- Copy all legacy fields into the new `GameSave`.
- Backfill `completed_bits`: for each `i` in `0..min(legacy.current_puzzle_index, 64)`, set bit `i`. Rationale: the linear progression guarantees the player completed puzzles 0..N-1 to reach puzzle N.
- Zero `puzzle_best_time` and `puzzle_best_tries` (no record for unseen-by-v1.2 puzzles — they show as "—" in the puzzle-select stats).
- Recompute checksum + write back as v2 immediately so the next load is a fast-path full read.

Why this matters: real cartridge users (and mGBA `.sav` file users) have committed save state. Wiping it on first v1.2 boot would lose their progress. The migration is a one-time, idempotent operation.

### Music engine extension

Current `MusicTrack` is single-channel (CH1 via `MusicStep`). The CH2 extension uses an optional parallel array:

```c
typedef struct {
    const MusicStep *steps;       // CH1: existing
    const uint8_t   *ch2_notes;   // NEW: optional. NULL = no harmony. If non-NULL, must be `length` elements long. Each entry is a MUSIC_NOTE_* value (or MUSIC_NOTE_REST).
    uint8_t          length;
    uint8_t          loop_start;
} MusicTrack;
```

`music_tick()` advances both channels in lockstep — CH2 step transitions happen at the same frame as CH1. Each CH2 note plays for the SAME `duration_frames` as the corresponding CH1 step. This is a deliberate simplification (real chiptune music supports independent voices with different timings); for an 8-bar title loop, lockstep harmony is musically sufficient.

Channel reservation table:
- **CH1**: music melody (existing) OR SFX `sfx_correct` / `sfx_wrong` / `sfx_win` / `sfx_lose` / `sfx_skip` / `sfx_stinger`. When SFX fires during music, it overrides the channel briefly — music resumes when the next note tick arrives.
- **CH2**: SFX `sfx_move` / `sfx_select` / `sfx_deselect` (existing) AND NEW music harmony. The harmony writes happen in `music_tick` (VBlank), SFX writes happen in main loop (input handlers). VBlank interrupt boundary means harmony writes can't be interrupted mid-write by SFX. SFX timing wins (brief "click"), then harmony resumes on next music_tick boundary.
- **CH4**: noise SFX (`sfx_reject`) — unchanged.

### Scene flow with puzzle-select

New scene `SCENE_PUZZLE_SELECT` slots into the existing scene table:

```
TITLE → (new option "PUZZLES") → PUZZLE_SELECT → (B) → TITLE
                                              ↓ (A on completed puzzle)
                                            PLAY (replay mode)
                                              ↓ WIN
                                            PUZZLE_SELECT (return to grid)
```

Replay mode distinction: when entered from `PUZZLE_SELECT`, scene_play loads that specific puzzle (not `current_puzzle_index`). The transient `replay_puzzle_index` lives in `scene_handoff.h` alongside `last_puzzle_result`. On WIN from a replay, transitions back to `PUZZLE_SELECT` (not the next puzzle in linear flow). On LOSE from a replay, same back-to-select on completion.

This means scene_play needs to know whether it's in linear or replay mode. Use the same handoff pattern: `extern bool is_replay_session` in scene_handoff.h, set by PUZZLE_SELECT before transition, read by scene_play in init.

### Why tiered celebrations rather than one big ALL_DONE

ALL_DONE is hit by very few players (5+ hours of play). Every-5-puzzles milestones are hit by anyone who plays for ~30 minutes. Engineering effort should follow player attention. Small CH1 fanfares + text flourishes for the frequent milestones; ALL_DONE gets a longer chiptune + scrolling text but no big animation work (per user feedback).

---

## Phase 1 — Save struct v2 migration + per-puzzle stats capture

**Execution Status:** ⬜ NOT STARTED

**Goal**: Extend `GameSave` with `completed_bits`, `puzzle_best_time[]`, `puzzle_best_tries[]`. Bump `SAVE_VERSION` to 2. Add v1→v2 migration path. Capture per-puzzle records when winning. End state: existing players' progress is preserved on first v1.2 boot; new wins update per-puzzle records visible later in Phase 7's puzzle-select screen.

**Why this is Phase 1:** Every later phase depends on these data shapes existing. Phase 7's puzzle-select reads `completed_bits` + records; Phase 3's tiered celebrations check `puzzles_solved_total` (already exists) and `current_streak` (already exists) but the codepath that updates them must NOT clobber the new fields.

### Task 1.1: Add MAX_PUZZLES_SUPPORTED constant + extend GameSave struct

**Files:**
- Modify: `src/game/puzzles_types.h` — add `MAX_PUZZLES_SUPPORTED` constant
- Modify: `src/game/game_state.h` — bump SAVE_VERSION, extend GameSave struct
- Modify: `src/engine/save.c` — update compile-time size assertions to match new struct

- [ ] **Step 1: Add MAX_PUZZLES_SUPPORTED to puzzles_types.h**

Open `src/game/puzzles_types.h`. After the existing `#define`s but before the struct, add:

```c
// Compile-time upper bound on puzzle count. Save struct allocates space
// for per-puzzle records up to this many puzzles. Must be a multiple of
// 8 (completed_bits is a uint8_t array). Currently 64 — supports up to
// 64 puzzles without a save migration. NUM_PUZZLES (runtime, set by
// codegen) MUST be <= MAX_PUZZLES_SUPPORTED.
#define MAX_PUZZLES_SUPPORTED 64
```

- [ ] **Step 2: Update GameSave struct in game_state.h**

Open `src/game/game_state.h`. Replace the existing `GameSave` struct + `SAVE_VERSION` define with the new v2 layout:

```c
// GameSave v2 — 220 bytes, persisted to SRAM at 0xA000.
// v1.0/v1.1 used a 20-byte v1 layout (still readable for migration).
// Field order is load-bearing for SRAM-format compatibility.
typedef struct {
    // --- v1 fields (unchanged offsets 0..19) ---
    uint8_t  magic[4];                 // [0..3]   "GBCX"
    uint8_t  version;                  // [4]      2 (was 1 in v1.0/v1.1)
    uint8_t  current_puzzle_index;     // [5]      0..NUM_PUZZLES-1
    uint8_t  current_puzzle_fails;     // [6]      0..3
    uint8_t  puzzles_solved_total;     // [7]
    uint8_t  puzzles_skipped_total;    // [8]
    uint8_t  current_streak;           // [9]
    uint8_t  best_streak;              // [10]
    uint16_t total_tries_used;         // [11..12]
    uint8_t  ip_tries_remaining;       // [13]     0 = no in-progress
    uint8_t  ip_groups_solved;         // [14]
    uint16_t ip_selected_mask;         // [15..16]
    uint16_t ip_elapsed_seconds;       // [17..18]
    uint8_t  v1_checksum;              // [19]     legacy checksum byte position
                                       //          (kept for layout compatibility;
                                       //          new checksum covers full struct)
    // --- v2 additions ---
    uint8_t  completed_bits[MAX_PUZZLES_SUPPORTED / 8];   // [20..27]  bitmap, 1 bit per puzzle
    uint16_t puzzle_best_time[MAX_PUZZLES_SUPPORTED];     // [28..155] seconds, 0 = no record
    uint8_t  puzzle_best_tries[MAX_PUZZLES_SUPPORTED];    // [156..219] tries used (1..4), 0 = no record (sentinel)
    uint8_t  checksum;                                    // [220]    XOR of bytes [0..219], excluding v1_checksum
} GameSave;
```

Wait — this puts checksum at offset 220, but the struct must be a power-of-2 size for clean alignment. Let me revise: keep total size at exactly 224 bytes by adding 3 reserved bytes.

Actually, GameSave doesn't need power-of-2 alignment for SRAM access (SRAM is byte-addressable). 220 bytes is fine. But to leave headroom for v3 without another migration, I'll add 4 reserved bytes:

```c
typedef struct {
    // --- v1 fields (unchanged offsets 0..19) ---
    uint8_t  magic[4];                 // [0..3]   "GBCX"
    uint8_t  version;                  // [4]      2 (was 1 in v1.0/v1.1)
    uint8_t  current_puzzle_index;     // [5]
    uint8_t  current_puzzle_fails;     // [6]
    uint8_t  puzzles_solved_total;     // [7]
    uint8_t  puzzles_skipped_total;    // [8]
    uint8_t  current_streak;           // [9]
    uint8_t  best_streak;              // [10]
    uint16_t total_tries_used;         // [11..12]
    uint8_t  ip_tries_remaining;       // [13]
    uint8_t  ip_groups_solved;         // [14]
    uint16_t ip_selected_mask;         // [15..16]
    uint16_t ip_elapsed_seconds;       // [17..18]
    uint8_t  v1_checksum;              // [19]    legacy slot — ignored in v2 (v2 uses checksum at offset 223)
    // --- v2 additions ---
    uint8_t  completed_bits[MAX_PUZZLES_SUPPORTED / 8];   // [20..27]
    uint16_t puzzle_best_time[MAX_PUZZLES_SUPPORTED];     // [28..155]
    uint8_t  puzzle_best_tries[MAX_PUZZLES_SUPPORTED];    // [156..219]
    uint8_t  reserved[3];                                 // [220..222] v3 headroom
    uint8_t  checksum;                                    // [223]      XOR of bytes [0..222]
} GameSave;
```

Total: 224 bytes.

Update `SAVE_VERSION` define from 1 to 2:

```c
#define SAVE_VERSION 2
```

- [ ] **Step 3: Update compile-time size assertions in save.c**

Open `src/engine/save.c`. The existing assertions:

```c
typedef char _check_GameSave_size[sizeof(GameSave) == 20 ? 1 : -1];
typedef char _check_cpi_offset[offsetof(GameSave, current_puzzle_index) == 5 ? 1 : -1];
typedef char _check_ip_tries_offset[offsetof(GameSave, ip_tries_remaining) == 13 ? 1 : -1];
typedef char _check_checksum_offset[offsetof(GameSave, checksum) == 19 ? 1 : -1];
```

Replace with:

```c
typedef char _check_GameSave_size[sizeof(GameSave) == 224 ? 1 : -1];
typedef char _check_cpi_offset[offsetof(GameSave, current_puzzle_index) == 5 ? 1 : -1];
typedef char _check_ip_tries_offset[offsetof(GameSave, ip_tries_remaining) == 13 ? 1 : -1];
typedef char _check_v1_checksum_offset[offsetof(GameSave, v1_checksum) == 19 ? 1 : -1];
typedef char _check_completed_offset[offsetof(GameSave, completed_bits) == 20 ? 1 : -1];
typedef char _check_best_time_offset[offsetof(GameSave, puzzle_best_time) == 28 ? 1 : -1];
typedef char _check_best_tries_offset[offsetof(GameSave, puzzle_best_tries) == 156 ? 1 : -1];
typedef char _check_v2_checksum_offset[offsetof(GameSave, checksum) == 223 ? 1 : -1];
```

If the asserts fail at compile time with "negative array size" errors, SDCC inserted padding. Diagnose by adding `printf("%d\n", (int)offsetof(GameSave, completed_bits))` to a test main and adjusting expected offsets to match observed.

- [ ] **Step 4: Build to verify struct layout matches assertions**

```bash
make 2>&1 | tail -10
```

Expected: clean build, no "negative array size" errors. If errors, the struct has unexpected padding — fix by reordering fields (uint16 fields should be at even offsets, uint8 fills the gaps).

- [ ] **Step 5: Commit**

```bash
git add src/game/puzzles_types.h src/game/game_state.h src/engine/save.c
git commit -m "v1.2.1.1: GameSave v2 — extend with completed_bits + per-puzzle records

MAX_PUZZLES_SUPPORTED = 64 (multiple of 8 for bitmap, future-proof
to ~88-puzzle no-banking ceiling). New fields: completed_bits[8]
(bitmap), puzzle_best_time[64] (uint16 seconds), puzzle_best_tries[64]
(uint8). Struct grows 20 → 224 bytes (still tiny vs 8KB SRAM).
SAVE_VERSION bumped 1 → 2. Compile-time offset asserts updated for
the new layout. Migration path lands in 1.2."
```

### Task 1.2: Implement v1 → v2 save migration

**Files:**
- Modify: `src/engine/save.c` (`save_load` function)

**Approach:** When `save_load` reads SRAM and finds valid magic but `version == 1`, treat as a legacy v1 save. Compute the legacy checksum over the first 20 bytes (using the v1 checksum algorithm). If valid, copy v1 fields into the new struct, backfill `completed_bits` from `current_puzzle_index`, zero the new fields, save back as v2.

- [ ] **Step 1: Add the LegacyGameSaveV1 type to save.c**

In `src/engine/save.c`, after the existing compile-time assertions, add the legacy struct definition:

```c
// Legacy v1 (v1.0/v1.1) save layout — 20 bytes. Read separately for
// migration; do NOT use for any new code. Field order MUST match the
// original v1 struct exactly.
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

// Compute the v1 checksum (XOR of bytes [0..18]).
static uint8_t compute_v1_checksum(const LegacyGameSaveV1 *s) {
    const uint8_t *p = (const uint8_t *)s;
    uint8_t x = 0;
    for (uint8_t i = 0; i < 19; i++) x ^= p[i];
    return x;
}
```

- [ ] **Step 2: Update compute_checksum to cover the v2 byte range**

The existing `compute_checksum` covers bytes 0..18. v2's checksum covers bytes 0..222. Replace:

```c
static uint8_t compute_checksum(const GameSave *s) {
    const uint8_t *p = (const uint8_t *)s;
    uint8_t x = 0;
    for (uint16_t i = 0; i < 223; i++) x ^= p[i];
    return x;
}
```

Note: changed counter type from `uint8_t` to `uint16_t` since 223 fits in uint8_t but the loop bound check `i < 223` works either way. uint16 is safer and matches the new struct size.

- [ ] **Step 3: Add the migration function**

After `compute_v1_checksum`, before `save_reset`, add:

```c
// Migrate a validated v1 save to a v2 GameSave in `out`. The completed
// bitmap is backfilled from current_puzzle_index — linear progression
// guarantees puzzles 0..N-1 were completed to reach puzzle N. Per-puzzle
// records are zeroed (no historical data — players who replay see "—"
// as the previous-best until they set a new record).
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

    // Backfill completed_bits: mark puzzles 0..(current_puzzle_index-1) complete.
    // Use min() vs MAX_PUZZLES_SUPPORTED to avoid out-of-bounds bit set if the
    // legacy save somehow has current_puzzle_index > 64.
    uint8_t backfill_count = legacy->current_puzzle_index;
    if (backfill_count > MAX_PUZZLES_SUPPORTED) backfill_count = MAX_PUZZLES_SUPPORTED;
    for (uint8_t i = 0; i < backfill_count; i++) {
        out->completed_bits[i >> 3] |= (uint8_t)(1u << (i & 7));
    }
    // puzzle_best_time and puzzle_best_tries left as 0 (sentinel for "no record").

    out->checksum = compute_checksum(out);
}
```

- [ ] **Step 4: Rewrite save_load to handle both v1 and v2**

Replace the existing `save_load` function with:

```c
bool save_load(GameSave *out) {
    // First, read just the v1 header bytes to check magic + version.
    // We can't memcpy sizeof(GameSave) without first knowing whether
    // the SRAM contains a v1 save (only 20 bytes valid) or a v2 save
    // (224 bytes valid). Read the 20-byte legacy view first.
    LegacyGameSaveV1 legacy;
    ENABLE_RAM_MBC1;
    SWITCH_RAM_MBC1(0);
    memcpy(&legacy, (void *)0xA000, sizeof(LegacyGameSaveV1));

    // Magic check — must match in ALL save versions (v1 and v2 share magic).
    if (legacy.magic[0] != SAVE_MAGIC_0 || legacy.magic[1] != SAVE_MAGIC_1
     || legacy.magic[2] != SAVE_MAGIC_2 || legacy.magic[3] != SAVE_MAGIC_3) {
        DISABLE_RAM_MBC1;
        save_reset(out);
        save_store(out);
        return false;
    }

    if (legacy.version == 1) {
        // v1 save — validate v1 checksum, then migrate.
        if (compute_v1_checksum(&legacy) != legacy.checksum) {
            DISABLE_RAM_MBC1;
            save_reset(out);
            save_store(out);
            return false;
        }
        DISABLE_RAM_MBC1;
        migrate_v1_to_v2(&legacy, out);
        save_store(out);  // immediately persist as v2 so next load is fast-path
        return true;
    }

    if (legacy.version == 2) {
        // v2 save — re-read the full struct now that we know the size.
        GameSave tmp;
        memcpy(&tmp, (void *)0xA000, sizeof(GameSave));
        DISABLE_RAM_MBC1;

        if (compute_checksum(&tmp) != tmp.checksum) {
            save_reset(out);
            save_store(out);
            return false;
        }

        // Sanity bounds (same as v1; still apply for v2).
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

    // Unknown future version — wipe and start fresh. (When v3 lands,
    // add another `if (legacy.version == 2)` branch above for a v2→v3
    // migration; this default-branch wipe is reserved for truly unknown
    // versions where migration intent is unclear.)
    DISABLE_RAM_MBC1;
    save_reset(out);
    save_store(out);
    return false;
}
```

- [ ] **Step 5: Build + verify clean compile**

```bash
make 2>&1 | tail -5
```

Expected: clean build. If `memset` is undefined, ensure `<string.h>` is included (it is — already there).

- [ ] **Step 6: Manual migration test in mGBA**

Because save migration only matters for users with existing v1 saves, automated host testing isn't practical (the test environment doesn't have a real `.sav` file from a prior version). Manual test procedure:

1. **Set up a v1 save**: check out the v1.1 release commit (or the immediately-prior commit on main), build, run in mGBA, play a few puzzles to set `current_puzzle_index` to 3 or so. Close mGBA — this writes the .sav file with v1 format.
2. **Switch back to v1.2 work-in-progress**: rebuild with the new GameSave struct + migration code.
3. **Open the .sav-bearing ROM in mGBA**: the migration should fire on first save_load (TITLE scene init). Verify that:
   - The TITLE menu shows CONTINUE (current_puzzle_index preserved as 3)
   - puzzles_solved_total preserved (visible on TITLE stats line)
   - best_streak preserved
   - No "starting fresh" wipe
4. Save state in mGBA, exit, peek at .sav file (should now be 224 bytes vs 20):
   ```bash
   ls -la *.sav 2>/dev/null || ls -la "%APPDATA%/mGBA/*.sav" 2>/dev/null
   ```

- [ ] **Step 7: Commit**

```bash
git add src/engine/save.c
git commit -m "v1.2.1.2: v1→v2 save migration — backfill completed_bits from current_puzzle_index

Existing v1.0/v1.1 player save data is preserved on first v1.2 boot.
Linear progression invariant means puzzles 0..(current_puzzle_index-1)
are inferred complete and marked in completed_bits. Per-puzzle records
zeroed (no historical record — replay sets new bests).

LegacyGameSaveV1 + compute_v1_checksum keep the v1 read path readable.
On v1 detection: migrate + save_store immediately so next load goes
through the fast v2 path (no double migration).

Unknown future versions still wipe (deferred — when v3 lands, add a
v2→v3 branch)."
```

### Task 1.3: Update scene_play to record per-puzzle bests on WIN

**Files:**
- Modify: `src/game/scene_play.c` — when winning a group that finishes the puzzle, update completed_bits + best stats before transitioning to WIN

- [ ] **Step 1: Find the WIN-transition block in scene_play.c**

In scene_play.c, the WIN transition logic is in `play_update` around line 359+ (the `if (ps.groups_solved == 0x0F)` block). It currently captures `last_puzzle_result` for the WIN scene's display, increments save fields, advances `current_puzzle_index`, and transitions.

- [ ] **Step 2: Add completed_bits + per-puzzle record update before save_store**

Insert before the existing `pg_save.puzzles_solved_total++;` line:

```c
                // v1.2: record this puzzle as completed + update per-puzzle bests.
                // Always set the completed bit (idempotent if already set).
                // Update best_time / best_tries only if this attempt is better
                // than the existing record (best_tries == 0 sentinel means
                // "no record" so any value beats it).
                {
                    uint8_t puzzle_idx = pg_save.current_puzzle_index;
                    uint8_t tries_used = (uint8_t)(4 - ps.tries_remaining);
                    if (puzzle_idx < MAX_PUZZLES_SUPPORTED) {
                        pg_save.completed_bits[puzzle_idx >> 3] |=
                            (uint8_t)(1u << (puzzle_idx & 7));

                        // Time: update if new is faster, OR if no prior record.
                        if (pg_save.puzzle_best_time[puzzle_idx] == 0
                         || ps.elapsed_seconds < pg_save.puzzle_best_time[puzzle_idx]) {
                            pg_save.puzzle_best_time[puzzle_idx] = ps.elapsed_seconds;
                        }
                        // Tries: update if new is fewer, OR if no prior record.
                        // Sentinel: 0 means "no record" (since legitimate tries
                        // range is 1..4).
                        if (pg_save.puzzle_best_tries[puzzle_idx] == 0
                         || tries_used < pg_save.puzzle_best_tries[puzzle_idx]) {
                            pg_save.puzzle_best_tries[puzzle_idx] = tries_used;
                        }
                    }
                }
```

- [ ] **Step 3: Build**

```bash
make 2>&1 | tail -3
```

- [ ] **Step 4: Manual verification in mGBA**

Play and complete puzzle 1 (any group). Quit to TITLE. Power-cycle mGBA (File → Reset). Boot — the save should load cleanly (v2 fast path). Play puzzle 1 again (would require puzzle-select scene — not built yet — so this test mostly verifies the write didn't crash; the read-back verification happens in Phase 7).

Alternative verification: use mGBA's memory viewer (Tools → "Game state" or "Memory") to inspect SRAM at 0xA000. After winning puzzle 1, bit 0 of offset 20 (completed_bits[0]) should be 1. puzzle_best_time[0] at offsets 28-29 (uint16 little-endian) should be non-zero (matching the seconds taken).

- [ ] **Step 5: Commit**

```bash
git add src/game/scene_play.c
git commit -m "v1.2.1.3: scene_play records per-puzzle bests on WIN transition

When all 4 groups solved: set the completed_bits bit + update
puzzle_best_time/puzzle_best_tries if the attempt beats the prior
record. Sentinel: best_tries == 0 means 'no record' (legitimate range
is 1..4). Per-puzzle records become visible in Phase 7's puzzle-select
scene."
```

### Phase 1 group review

After Tasks 1.1, 1.2, 1.3:

Review the batch from multiple perspectives. Minimum 3 review rounds:
1. **Build sanity**: clean rebuild succeeds, all compile-time assertions pass, `make test` still passes (60+ tests).
2. **Migration correctness**: manually-tested v1 save preserves CONTINUE state on first v1.2 boot; SRAM at 0xA000 shows expected v2 layout after migration (224 bytes, version byte = 2).
3. **No regression on v1.0/v1.1 gameplay**: solve a puzzle on a fresh save — WIN scene shows correct stats; next puzzle loads; save state persists across mGBA restart.

If round 3 still finds issues, keep going until clean.

---

## Phase 2 — Cascade-bar slide-in (scene_win + scene_lose)

**Execution Status:** ⬜ NOT STARTED

**Goal**: When tier bars cascade in on WIN (Plan B's ANIM_BAR_CASCADE) and LOSE (ANIM_LOSE_REVEAL), each bar slides in horizontally from off-screen left rather than popping in. Visual polish only — animation duration and bar count unchanged.

**Why this is its own phase**: pure visual polish; touches scene_win.c and scene_lose.c but no shared engine code. Easy to revert if it doesn't feel right.

**Approach**: GBDK's `SCX_REG` shifts the entire background horizontally. We can't shift a single row independently — SCX affects ALL rows. So a per-bar slide-in trick using SCX wouldn't work without HBlank-triggered SCX changes (LCDC STAT interrupt), which is too much complexity for this polish item.

**Alternative approach**: animate the bar's tile X positions in the tilemap. Each bar starts with its tiles 20 columns off-screen (so it's invisible), then per frame the bar tiles shift right by 4 columns until they're in place. Takes 5 frames to land. Use the SAME tile layout helper but with an X offset.

Actually the simplest and cleanest: each bar slides in over ~5 frames by progressively painting more of its tiles per frame. Frame 1: paint columns 17-19 only. Frame 2: paint columns 14-19. Frame 3: 11-19. Frame 4: 8-19. Frame 5: 0-19 (full bar). This gives the visual impression of a "slide in from right" without SCX trickery.

Hmm — "slide in from LEFT" vs "RIGHT" — the user said "slide in" without direction. Right-to-left feels more natural (left-to-right would make text appear backwards reading). Let me do left-to-right (text appears reading-direction-correct as it lands). Frame 1: column 0 only. Frame 5: full bar.

Or simpler still: keep the cascade timing identical (20 frames per bar in scene_win), but during the first 5 frames of each bar's "slot", animate it left-to-right. Frames 6-19: bar is fully painted, no change. This means we change scene_win's per-bar rendering, not the cascade timing.

### Task 2.1: scene_win — slide-in animation for cascading bars

**Files:**
- Modify: `src/game/scene_win.c` — extend render_bar to support a "slide progress" parameter

**Approach detail**: render_bar gains an optional parameter `reveal_cols` (0..20). If reveal_cols < 20, only paint columns `0..reveal_cols-1` of the bar; the rest stay as whatever was there before (typically background/cleared). When reveal_cols == 20, paint full bar.

Wait — that means partial painting. Since render_clear was moved to win_init and bars are painted additively, partial bar painting would leave the right side of the bar as "no tile" (zero / blank). That's fine — the bar appears to slide in.

But for the cascade timing: currently bar cascade advances by ONE bar every 20 frames. We want the bar to ALSO slide in within its 20-frame slot. Subdivide: the slide takes 5 frames (4-column advance per frame), then bar is "fully landed" for the remaining 15 frames before the next bar starts.

scene_win currently tracks `cascade_step` (0..4 — number of bars fully revealed). Add `slide_progress` (0..20, columns painted of the *current* bar being revealed). Re-derive `slide_progress` from elapsed frames just like cascade_step.

- [ ] **Step 1: Add slide_progress state to scene_win.c**

In scene_win.c after the existing `static` declarations, add:

```c
static uint8_t slide_cols;        // 0..20 — columns of the currently-sliding bar that are painted
static uint8_t last_rendered_slide_cols;  // diff against slide_cols in win_render
```

In `win_init`, reset both:

```c
slide_cols = 0;
last_rendered_slide_cols = 0;
```

- [ ] **Step 2: Drive slide_cols from elapsed frames in win_update**

In `win_update`, inside the `if (anim_current() == ANIM_BAR_CASCADE)` block, after the existing `cascade_step` computation, add:

```c
        // Compute slide_cols for the bar currently sliding in.
        // Each bar gets 20 frames total. First 5 frames = slide-in
        // (4 columns per frame: 4, 8, 12, 16, 20). Remaining 15 frames
        // = bar is fully painted; slide_cols stays at 20 until the
        // next bar starts.
        uint16_t bar_local = (uint16_t)(elapsed % 20);  // 0..19 frames into current bar
        uint8_t new_slide;
        if (bar_local < 5) {
            new_slide = (uint8_t)((bar_local + 1) * 4);  // 4, 8, 12, 16, 20
        } else {
            new_slide = 20;
        }
        if (new_slide != slide_cols) {
            slide_cols = new_slide;
            redraw_needed = true;
        }
```

- [ ] **Step 3: Modify render_bar to take a reveal_cols parameter**

Change the `render_bar` signature:

```c
static void render_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y, uint8_t reveal_cols) {
```

Inside render_bar, gate each tile write on `x < reveal_cols`:

```c
static void render_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y, uint8_t reveal_cols) {
    uint8_t pattern_tile = (uint8_t)(UI_TILE_PATTERN_BASE + tier);
    for (uint8_t x = 0; x < 3; x++) {
        if (x < reveal_cols) render_set_tile(x, y, pattern_tile);
    }
    for (uint8_t x = 17; x < 20; x++) {
        if (x < reveal_cols) render_set_tile(x, y, pattern_tile);
    }
    for (uint8_t x = 3; x < 17; x++) {
        if (x < reveal_cols) render_set_tile(x, y, UI_TILE_SOLID_DARK);
    }

    // Overlay category name only when bar is fully revealed (avoid
    // letters appearing chopped during slide-in).
    if (reveal_cols >= 20) {
        const char *name = puzzle->category_names[tier];
        uint8_t name_len = 0;
        while (name[name_len] && name_len < 14) name_len++;
        uint8_t text_x = (uint8_t)(3 + (14 - name_len) / 2);
        render_text_inv(text_x, y, name);
    }

    // Phase 3 (Plan D): tier palette write. Only paint palette for
    // currently-revealed columns to avoid coloring blank space.
    uint8_t palette = (uint8_t)(GBC_PAL_TIER_YELLOW + tier);
    for (uint8_t x = 0; x < SCREEN_TILES_W && x < reveal_cols; x++) {
        render_set_tile_palette(x, y, palette);
    }
}
```

- [ ] **Step 4: Update win_render to pass slide_cols for the currently-sliding bar**

In `win_render`, the existing loop renders already-revealed bars with reveal_cols=20 (full) and the currently-sliding bar with reveal_cols=slide_cols:

```c
    while (last_rendered_cascade_step < cascade_step
        && last_rendered_cascade_step < 4) {
        render_bar(puzzle,
                   last_rendered_cascade_step,
                   (uint8_t)(3 + last_rendered_cascade_step),
                   20);  // fully revealed
        last_rendered_cascade_step++;
    }

    // The currently-sliding bar (if any) needs an incremental redraw
    // as slide_cols grows. Only re-render if slide_cols changed AND
    // there's an in-progress bar (cascade_step < 4 means there's a
    // next bar still revealing).
    if (cascade_step < 4 && last_rendered_slide_cols != slide_cols) {
        render_bar(puzzle, cascade_step, (uint8_t)(3 + cascade_step), slide_cols);
        last_rendered_slide_cols = slide_cols;
    }
```

Also reset `last_rendered_slide_cols = 0` when `cascade_step` advances (so the next bar's slide starts from scratch):

```c
    while (last_rendered_cascade_step < cascade_step
        && last_rendered_cascade_step < 4) {
        render_bar(puzzle, last_rendered_cascade_step,
                   (uint8_t)(3 + last_rendered_cascade_step), 20);
        last_rendered_cascade_step++;
        last_rendered_slide_cols = 0;  // next bar starts fresh
    }
```

- [ ] **Step 5: Build**

```bash
make 2>&1 | tail -3
```

- [ ] **Step 6: Verify visually in mGBA**

Solve all 4 groups of any puzzle. On WIN scene, each tier bar should now visibly slide in from the left over ~5 frames before snapping fully into place + the category-name text appearing.

- [ ] **Step 7: Commit**

```bash
git add src/game/scene_win.c
git commit -m "v1.2.2.1: scene_win cascade bars slide in over 5 frames (4 cols/frame)

Each bar's 20-frame cascade slot now subdivides into 5 frames of
left-to-right slide (4 columns per frame) + 15 frames fully revealed.
Category name + palette writes only fire at full reveal to avoid
chopped-letter artifacts during slide. render_bar takes a new
reveal_cols parameter (existing callers pass 20)."
```

### Task 2.2: scene_lose — same slide-in for ANIM_LOSE_REVEAL

**Files:**
- Modify: `src/game/scene_lose.c` — apply the same slide-in pattern to its `render_bar`

- [ ] **Step 1: Apply the same pattern to scene_lose.c**

scene_lose has a parallel `render_bar(puzzle, tier, y)` function and a parallel reveal animation (`reveal_step` driven by `ANIM_LOSE_REVEAL`). Apply the same slide_cols tracking + render_bar(reveal_cols) signature change. The reveal animation timing in scene_lose is identical to scene_win's (4 bars × 20 frames = 80 frames total), so the same 5-frames-slide formula applies.

Add to scene_lose.c statics:

```c
static uint8_t slide_cols;
static uint8_t last_rendered_slide_cols;
```

Reset in `lose_init`:

```c
slide_cols = 0;
last_rendered_slide_cols = 0;
```

In `lose_update`'s `ANIM_LOSE_REVEAL` block, after the `reveal_step` computation:

```c
        uint16_t bar_local = (uint16_t)(elapsed % 20);
        uint8_t new_slide;
        if (bar_local < 5) {
            new_slide = (uint8_t)((bar_local + 1) * 4);
        } else {
            new_slide = 20;
        }
        if (new_slide != slide_cols) {
            slide_cols = new_slide;
            redraw_needed = true;
        }
```

Update `render_bar` signature + body to match scene_win's pattern (reveal_cols parameter, gated tile writes, name+palette only at full reveal).

Update `lose_render` to call `render_bar(puzzle, i, y, 20)` for already-revealed bars and `render_bar(puzzle, reveal_step, y, slide_cols)` for the currently-sliding one. (scene_lose uses `render_clear` + full re-render on redraw, so the diff-tracking trick from scene_win isn't strictly needed — but it doesn't hurt and keeps the visual pattern identical.)

Actually scene_lose's `lose_render` uses `render_clear` then renders bars 0..reveal_step-1 fully, then leaves bar at reveal_step partially. Simpler: render bars 0..reveal_step-1 with reveal_cols=20, then render bar at reveal_step with reveal_cols=slide_cols if cascade still in progress.

```c
    for (uint8_t i = 0; i < 4 && i < reveal_step; i++) {
        render_bar(puzzle, i, (uint8_t)(3 + i), 20);  // fully revealed
    }
    // Currently-sliding bar (if any).
    if (reveal_step < 4 && slide_cols > 0) {
        render_bar(puzzle, reveal_step, (uint8_t)(3 + reveal_step), slide_cols);
    }
```

- [ ] **Step 2: Build + verify visually in mGBA**

```bash
make 2>&1 | tail -3
```

Trigger LOSE (submit 4 wrong groups). Each tier bar in the answer reveal should slide in from the left over 5 frames.

- [ ] **Step 3: Commit**

```bash
git add src/game/scene_lose.c
git commit -m "v1.2.2.2: scene_lose cascade bars slide in (same pattern as scene_win)"
```

### Phase 2 group review

1. **Visual consistency**: both WIN and LOSE bar reveals have the same slide-in character.
2. **No flicker introduction**: slide_cols tracking uses the incremental render pattern (no render_clear on each slide step in scene_win). scene_lose does still render_clear per redraw — that's pre-existing and fine since the slide animation only redraws once per frame.
3. **Text readability preserved**: category-name text only paints at full reveal — no chopped-letter intermediate frames.

---

## Phase 3 — Tiered milestone celebrations (lifetime + streak)

**Execution Status:** ⬜ NOT STARTED

**Goal**: After WIN, if either (a) `puzzles_solved_total` is a multiple of 5, fire a small "lifetime milestone" celebration — brief text + CH1 fanfare. (b) `current_streak` is a multiple of 5, fire a bigger "streak milestone" celebration — bigger text + longer fanfare + brief animation. Both can fire on the same WIN (5-streak + 5-lifetime would show both back-to-back).

**Why**: ALL_DONE is too distant a payoff for most players. Every-5 milestones give the median player a satisfying mini-payoff during normal play.

**Approach**: scene_win's existing cascade + typewriter stats reveal is preserved. After all that completes (anim_is_playing() is false, all stats fully revealed), check save state for milestone hits. If lifetime % 5 == 0 AND lifetime > 0: render "MILESTONE: 5 SOLVED!" text overlay + play short fanfare. If streak % 5 == 0 AND streak > 0: render bigger "STREAK 5!" text + longer fanfare. Wait for player input (START) to dismiss → transition to PLAY (or PUZZLE_SELECT if replay).

### Task 3.1: Add milestone state to scene_win

**Files:**
- Modify: `src/game/scene_win.c` — add milestone tracking + new render state

- [ ] **Step 1: Add milestone state**

In scene_win.c, after the existing statics:

```c
// v1.2: tiered milestones. Set in win_init after save_load if puzzles
// solved/streak hit a multiple of 5. Rendered after stats fully revealed.
static bool milestone_lifetime_hit;   // puzzles_solved_total % 5 == 0 (and > 0)
static bool milestone_streak_hit;     // current_streak % 5 == 0 (and > 0)
static bool milestone_displayed;      // true once milestone text has rendered (one-shot flag)
static bool milestone_fanfare_played; // true once the fanfare SFX has fired (one-shot flag)
```

In `win_init`, set them based on the loaded save:

```c
    // Milestone detection. puzzles_solved_total was incremented by scene_play
    // before transitioning here, so the new total is what's in save.
    milestone_lifetime_hit = (win_save.puzzles_solved_total > 0
                           && win_save.puzzles_solved_total % 5 == 0);
    milestone_streak_hit   = (win_save.current_streak > 0
                           && win_save.current_streak % 5 == 0);
    milestone_displayed       = false;
    milestone_fanfare_played  = false;
```

- [ ] **Step 2: Add milestone rendering in win_render after stats reveal**

In `win_render`, after the existing stats typewriter loop:

```c
    // v1.2 milestone celebrations — render after all stats lines revealed.
    if (last_rendered_stats_step >= 6 && !milestone_displayed) {
        if (milestone_streak_hit) {
            // Bigger celebration on streak milestone — overlays the
            // bottom of the screen. STREAK has the larger payoff because
            // it's harder to achieve (no losses or skips between).
            char buf[21];
            sprintf(buf, "STREAK %d!", (int)win_save.current_streak);
            render_text(5, 16, buf);
            render_text(2, 17, "PRESS START!");
        } else if (milestone_lifetime_hit) {
            // Smaller celebration — overlays the menu hint row.
            char buf[21];
            sprintf(buf, "%d SOLVED!", (int)win_save.puzzles_solved_total);
            render_text(5, 16, buf);
            render_text(2, 17, "PRESS START!");
        }
        milestone_displayed = true;
    }
```

- [ ] **Step 3: Fire fanfare SFX in win_update**

Add a fanfare-trigger block in `win_update` that fires once after stats fully revealed AND a milestone is hit. The fanfare itself reuses existing SFX (small = sfx_select; big = sfx_correct or a new sfx_milestone — TBD by implementer based on what fits sound-wise; default to existing SFX to keep scope tight):

```c
    if (stats_step >= 6 && !milestone_fanfare_played) {
        if (milestone_streak_hit) {
            sfx_win();  // reuse the win fanfare for bigger streak celebration
        } else if (milestone_lifetime_hit) {
            sfx_correct();  // reuse the correct chime for smaller lifetime celebration
        }
        milestone_fanfare_played = true;
    }
```

Place this block AFTER the existing typewriter advancement but BEFORE the START/SELECT input handling.

- [ ] **Step 4: Suppress the existing "START NEXT / SELECT TITLE" text when milestone shows**

The existing stats typewriter renders at step 6:
```c
            case 6:
                render_text(2, 16, "START  NEXT");
                render_text(2, 17, "SELECT TITLE");
                break;
```

When a milestone is hit, those two lines collide with the milestone text. Skip them in the milestone case:

```c
            case 6:
                if (milestone_streak_hit || milestone_lifetime_hit) {
                    // Milestone text replaces the standard menu hints.
                    // No render call here — Phase 3.1 Step 2 renders the
                    // milestone text in its own block.
                } else {
                    render_text(2, 16, "START  NEXT");
                    render_text(2, 17, "SELECT TITLE");
                }
                break;
```

- [ ] **Step 5: Build + manually verify**

```bash
make 2>&1 | tail -3
```

To verify quickly without playing 5 puzzles: use mGBA's memory viewer to set `puzzles_solved_total` to 4, then win a puzzle (now total = 5). Or modify your save in mGBA. Lifetime milestone should fire. For streak milestone, set `current_streak` to 4 and win without skipping.

- [ ] **Step 6: Commit**

```bash
git add src/game/scene_win.c
git commit -m "v1.2.3: tiered milestone celebrations on scene_win

Lifetime milestone (puzzles_solved_total %5 == 0): small text overlay
('5 SOLVED!') + sfx_correct chime. Streak milestone (current_streak %5
== 0): bigger text ('STREAK 5!') + sfx_win fanfare. Both render after
the stats typewriter completes; replaces the standard menu hints when
active. Same START-to-continue input handler — milestone is purely
visual + audio overlay, doesn't gate anything."
```

### Phase 3 group review

1. **Milestones fire only when expected**: a non-milestone win (e.g., puzzles_solved_total = 7, current_streak = 7) shows NO milestone text — just the normal stats screen.
2. **Both milestones together work cleanly**: at puzzles_solved_total = 5 AND current_streak = 5, the streak milestone (bigger) wins the display (it's the more impressive achievement). Lifetime celebration is skipped to avoid double-celebration.
3. **No regression**: normal WIN flow (no milestone) renders identically to v1.1.

---

## Phase 4 — Music engine: CH2 harmony support

**Execution Status:** ⬜ NOT STARTED

**Goal**: Extend the music engine to support an optional CH2 harmony layer alongside the existing CH1 melody. Lockstep timing — CH2 note transitions happen at the same frame as CH1 transitions. Existing CH1-only tracks (title_theme) keep working unchanged with `ch2_notes = NULL`.

**Why**: Phase 5 needs this engine support before authoring the harmony for title_theme. Splitting engine work from content work lets the engine ship + get tested independently.

### Task 4.1: Extend MusicTrack + emit_note for CH2

**Files:**
- Modify: `src/engine/music.h` — add `ch2_notes` field to `MusicTrack`
- Modify: `src/engine/music.c` — emit CH2 notes alongside CH1 in `music_play` + `music_tick`

- [ ] **Step 1: Add ch2_notes to MusicTrack struct in music.h**

```c
typedef struct {
    const MusicStep *steps;       // CH1 melody (existing — unchanged)
    const uint8_t   *ch2_notes;   // OPTIONAL CH2 harmony. NULL = no harmony.
                                  // If non-NULL: MUST be `length` elements long
                                  // (same indexing as `steps`). Each entry is a
                                  // MUSIC_NOTE_* value (or MUSIC_NOTE_REST).
                                  // CH2 note transitions happen at the same
                                  // frame as the corresponding CH1 transition.
    uint8_t          length;
    uint8_t          loop_start;
} MusicTrack;
```

- [ ] **Step 2: Add emit_note_ch2 helper in music.c**

In `src/engine/music.c`, after the existing `emit_note` function, add a CH2 emitter:

```c
// Emit a note on CH2 (square 2). Same tone shape as music's CH1 melody
// (25% duty, volume 6, decay step 3) so the two voices blend rather
// than compete. CH2 register set: NR21 (duty), NR22 (envelope),
// NR23 (frequency low), NR24 (frequency high + trigger).
static void emit_note_ch2(uint8_t note) {
#ifdef __SDCC
    if (note == MUSIC_NOTE_REST) {
        NR22_REG = 0x00;  // volume 0 silences CH2
        NR24_REG = 0x80;  // trigger with silent envelope
        return;
    }
    if (note >= MUSIC_NOTE_COUNT) return;
    NR21_REG = 0x40;        // 25% duty, length unused
    NR22_REG = 0x53;        // volume 5 (one lower than CH1's 6 — harmony
                            //          sits below melody), decay, step 3
    NR23_REG = music_note_table[note].lo;
    NR24_REG = music_note_table[note].hi;
#else
    (void)note;
#endif
}
```

- [ ] **Step 3: Call emit_note_ch2 from music_play + music_tick**

In `music_play`, after the existing `emit_note(track->steps[0].note);`:

```c
    if (track->ch2_notes) {
        emit_note_ch2(track->ch2_notes[0]);
    }
```

In `music_tick`, after the existing `emit_note(active_track->steps[next_idx].note);`:

```c
    if (active_track->ch2_notes) {
        emit_note_ch2(active_track->ch2_notes[next_idx]);
    }
```

In `music_stop`, silence CH2 too:

```c
void music_stop(void) {
    active_track = 0;
    current_step_idx = 0xFF;
    frames_remaining = 0;
    emit_note(MUSIC_NOTE_REST);       // silence CH1
    emit_note_ch2(MUSIC_NOTE_REST);   // silence CH2
}
```

- [ ] **Step 4: Build**

```bash
make 2>&1 | tail -3
```

Expected: clean build. Existing title_theme.c initializer doesn't set `ch2_notes` → it stays NULL → CH2 emit calls are skipped → existing playback unchanged.

- [ ] **Step 5: Verify existing host tests still pass**

```bash
make test 2>&1 | tail -5
```

Expected: all 27 music tests still pass (the CH2 extension is transparent to the existing test code since `ch2_notes` defaults to NULL in all test fixtures).

- [ ] **Step 6: Add host tests for the CH2 path**

In `test/test_music.c`, add new test functions covering CH2:

```c
// ----- Test: CH2 path advances in lockstep with CH1 -----
static void test_ch2_lockstep_advance(void) {
    music_init();
    MusicStep ch1_steps[] = {
        { MUSIC_NOTE_C5, 2 },
        { MUSIC_NOTE_E5, 2 },
    };
    uint8_t ch2[] = { MUSIC_NOTE_G4, MUSIC_NOTE_C5 };
    MusicTrack t = { .steps = ch1_steps, .ch2_notes = ch2, .length = 2, .loop_start = 2 };
    music_play(&t);

    ASSERT(music_current_step() == 0);
    music_tick(); music_tick();  // step 0 frames done
    music_tick();  // advance to step 1
    ASSERT(music_current_step() == 1);
    // Both CH1 and CH2 should have emitted by now — host can't see the
    // hardware writes, but the state machine advance is verified.
}

// ----- Test: ch2_notes NULL is honored (no crash) -----
static void test_ch2_null_safe(void) {
    music_init();
    MusicStep ch1_steps[] = { { MUSIC_NOTE_C5, 1 } };
    MusicTrack t = { .steps = ch1_steps, .ch2_notes = 0, .length = 1, .loop_start = 1 };
    music_play(&t);  // should not crash on NULL ch2
    ASSERT(music_is_playing());
    music_tick();
    music_tick();  // end of track, no loop → stop
    ASSERT(!music_is_playing());
}
```

Wire both into `main` at the bottom of the existing test list:

```c
    test_ch2_lockstep_advance();
    test_ch2_null_safe();
```

- [ ] **Step 7: Run tests**

```bash
make test 2>&1 | tail -5
```

Expected: `Tests run: 29` (was 27, added 2) `Failed: 0`.

- [ ] **Step 8: Commit**

```bash
git add src/engine/music.h src/engine/music.c test/test_music.c
git commit -m "v1.2.4: music engine — optional CH2 harmony via parallel ch2_notes array

MusicTrack gains ch2_notes pointer. NULL (existing tracks) = no
harmony (CH2 path skipped). Non-NULL = lockstep with CH1, same step
boundaries. CH2 uses 25% duty + volume 5 (one lower than CH1's 6) so
harmony sits below melody. music_stop silences both channels. 2 new
host tests cover lockstep + NULL safety. All 29 tests pass."
```

### Phase 4 group review

1. **Backward compatibility**: existing title_theme plays identically (CH2 path skipped because ch2_notes is NULL).
2. **No SFX clobber**: when scene_title is active and harmony is playing on CH2, pressing D-pad fires `sfx_move` (which also writes CH2 registers). The brief SFX click overrides harmony, then next music_tick re-asserts CH2. Verify in mGBA — there should be a brief audible "blip" on SFX, then harmony resumes seamlessly.
3. **Host tests cover both paths**: lockstep advancement + NULL safety.

---

## Phase 5 — Title theme: CH2 harmony layer

**Execution Status:** ⬜ NOT STARTED

**Goal**: Add a CH2 harmony part to the existing 32-step title theme. End state: title screen plays a 2-voice melody (CH1 lead + CH2 harmony) instead of single-voice CH1.

**Approach**: Author a CH2 note array matching the existing 32-step CH1 melody. Simple harmony: thirds below the melody (C↔A, E↔C, G↔E, etc. mostly stays in C major). Some bars use stationary harmony notes (a held G or C) for "ambient" feel rather than chasing the melody.

### Task 5.1: Author title_theme CH2 array

**Files:**
- Modify: `src/game/title_theme.c` — add the CH2 array

- [ ] **Step 1: Add the CH2 note array to title_theme.c**

In `src/game/title_theme.c`, after the existing `TITLE_THEME_STEPS[32]` definition, add the harmony:

```c
// CH2 harmony layer — lockstep with TITLE_THEME_STEPS. Mostly thirds
// below the CH1 melody, with some held notes for ambient feel.
// 32 entries to match TITLE_THEME_STEPS length.
static const uint8_t TITLE_THEME_CH2[32] = {
    // Bar 1: CH1=C,E,G,C↑ → CH2 holds G (stable foundation)
    MUSIC_NOTE_G4, MUSIC_NOTE_G4, MUSIC_NOTE_G4, MUSIC_NOTE_G4,
    // Bar 2: CH1=G,E,C,rest → CH2 echoes the descent
    MUSIC_NOTE_E4, MUSIC_NOTE_C4, MUSIC_NOTE_G4, MUSIC_NOTE_REST,
    // Bar 3: CH1=C,E,G,C↑ (repeat opening) → CH2 thirds below
    MUSIC_NOTE_A4, MUSIC_NOTE_C5, MUSIC_NOTE_E5, MUSIC_NOTE_G5,
    // Bar 4: CH1=G,C↑,G,E → CH2 held E for tension
    MUSIC_NOTE_E5, MUSIC_NOTE_E5, MUSIC_NOTE_E5, MUSIC_NOTE_C5,
    // Bar 5: CH1=D,F,A,D (Dm arpeggio) → CH2 holds A
    MUSIC_NOTE_A4, MUSIC_NOTE_A4, MUSIC_NOTE_A4, MUSIC_NOTE_A4,
    // Bar 6: CH1=A,F,D,rest → CH2 descends in parallel
    MUSIC_NOTE_F4, MUSIC_NOTE_D4, MUSIC_NOTE_A4, MUSIC_NOTE_REST,
    // Bar 7: CH1=C,E,G,C↑ (return) → CH2 thirds below
    MUSIC_NOTE_A4, MUSIC_NOTE_C5, MUSIC_NOTE_E5, MUSIC_NOTE_G5,
    // Bar 8: CH1=C↑,G,E,C (resolution) → CH2 descends to C
    MUSIC_NOTE_E5, MUSIC_NOTE_C5, MUSIC_NOTE_G4, MUSIC_NOTE_C4,
};
```

Update the `TITLE_THEME` initializer to point to the new harmony:

```c
const MusicTrack TITLE_THEME = {
    .steps      = TITLE_THEME_STEPS,
    .ch2_notes  = TITLE_THEME_CH2,
    .length     = 32,
    .loop_start = 0,
};
```

- [ ] **Step 2: Build**

```bash
make 2>&1 | tail -3
```

- [ ] **Step 3: Verify in mGBA**

Open ROM in mGBA. Title screen should now play 2-voice music (CH1 melody + CH2 harmony layer). The harmony should sound musically supportive (not clashy). Move the d-pad to fire sfx_move on CH2 — harmony should briefly drop out for the click then resume.

- [ ] **Step 4: Commit (and iterate if it sounds off)**

```bash
git add src/game/title_theme.c
git commit -m "v1.2.5: title theme — CH2 harmony layer

32-note harmony lockstep with CH1 melody. Mostly thirds below the
lead with some held notes for ambient feel. Music engine's optional
ch2_notes parameter (from Phase 4) makes this purely additive — no
engine change needed."
```

If the harmony sounds wrong on first listen, treat it as polish iteration (similar to v1.1's `C6.tune` commit). Edit the CH2 array, rebuild, listen, repeat until it sounds right. Use commit subjects like `v1.2.5.tune: harmony adjust — <description>` for iterations.

### Phase 5 group review

1. **Musically coherent**: harmony supports the melody without clashing dissonance. If a specific bar sounds bad, isolate by setting that bar's CH2 entries to REST temporarily.
2. **SFX coexistence preserved**: cursor moves / select still work without breaking music character beyond the expected brief click.
3. **DMG-safe**: GBC isn't required — CH1 + CH2 work on DMG hardware identically.

---

## Phase 6 — Puzzle-start CH1 stinger

**Execution Status:** ⬜ NOT STARTED

**Goal**: When PLAY scene loads (fresh puzzle, NOT continuing in-progress), a brief 4-note CH1 fanfare plays. Gives the PLAY scene audio identity to pair with the music-having title.

**Approach**: Define a 4-step inline `STINGER` MusicTrack. Call `music_play(&STINGER)` in `play_init` ONLY if `pg_save.ip_tries_remaining == 0` (fresh puzzle, not in-progress restoration — restoring shouldn't fanfare since it's a continuation). Non-looping: `loop_start == length` makes the player stop after the last step.

### Task 6.1: Add the stinger track + scene_play wiring

**Files:**
- Create: `src/game/play_sfx.c` + `src/game/play_sfx.h` — holds the stinger track data
- Modify: `src/game/scene_play.c` — play stinger in play_init when starting fresh
- Modify: `Makefile` — add play_sfx.c to SRC

- [ ] **Step 1: Create play_sfx.h**

```c
#ifndef GAME_PLAY_SFX_H
#define GAME_PLAY_SFX_H

#include "../engine/music.h"

// 4-note CH1 fanfare played when a fresh puzzle starts. ~1 second total.
// Non-looping (loop_start == length).
extern const MusicTrack PLAY_STINGER;

#endif
```

- [ ] **Step 2: Create play_sfx.c**

```c
#include "play_sfx.h"

// 4 ascending notes — short, energetic. C → E → G → C↑ at quick tempo.
// Each note ~250ms (15 frames at 60fps).
static const MusicStep PLAY_STINGER_STEPS[4] = {
    { MUSIC_NOTE_C5, 15 },
    { MUSIC_NOTE_E5, 15 },
    { MUSIC_NOTE_G5, 15 },
    { MUSIC_NOTE_C6, 20 },  // last note slightly longer for landing feel
};

const MusicTrack PLAY_STINGER = {
    .steps      = PLAY_STINGER_STEPS,
    .ch2_notes  = 0,            // melody only — no harmony for a stinger
    .length     = 4,
    .loop_start = 4,            // non-looping
};
```

- [ ] **Step 3: Add to Makefile SRC list**

```makefile
SRC := src/main.c \
       src/engine/render.c src/engine/input.c src/engine/save.c \
       src/engine/sound.c src/engine/anim.c src/engine/music.c \
       src/game/puzzle_logic.c src/game/layout.c \
       src/game/scene_title.c src/game/scene_play.c src/game/scene_win.c \
       src/game/scene_lose.c src/game/scene_all_done.c \
       src/game/title_theme.c src/game/play_sfx.c
```

- [ ] **Step 4: Wire stinger in play_init**

In `src/game/scene_play.c`, add include at top:

```c
#include "play_sfx.h"
#include "../engine/music.h"
```

In `play_init`, after the existing init code (palette reset, save_load, layout compute), and AFTER the existing `set_sprite_data + set_sprite_tile + update_cursor_sprite` block:

```c
    // v1.2: stinger on fresh-puzzle start (not on in-progress restoration —
    // continuing shouldn't fanfare since it interrupts a player who's
    // already mid-thought).
    if (pg_save.ip_tries_remaining == 0) {
        music_play(&PLAY_STINGER);
    }
```

- [ ] **Step 5: Build**

```bash
make 2>&1 | tail -3
```

- [ ] **Step 6: Verify in mGBA**

Open ROM → NEW GAME → PLAY scene loads. Stinger should play (~1 second, 4 ascending notes). If you immediately solve a group (sfx_correct fires on CH1), the stinger gets cut off cleanly. Quit to TITLE, then choose CONTINUE on an in-progress puzzle — NO stinger should play.

- [ ] **Step 7: Commit**

```bash
git add src/game/play_sfx.h src/game/play_sfx.c src/game/scene_play.c Makefile
git commit -m "v1.2.6: puzzle-start CH1 stinger on fresh-puzzle PLAY init

4-note ascending fanfare (C5 → E5 → G5 → C6) plays when scene_play
loads with no in-progress state. Gated on ip_tries_remaining == 0 so
CONTINUE doesn't fanfare (would interrupt a mid-thought player).
Non-looping (single play, then silent CH1 until next note). New
play_sfx module isolates the stinger data from scene logic."
```

### Phase 6 group review

1. **Triggers correctly**: NEW GAME and RESTART trigger stinger; CONTINUE does NOT.
2. **No interference with title music**: scene_title's `music_stop()` in teardown silences CH1 before scene_play starts; stinger then plays cleanly.
3. **Stinger doesn't loop**: after 4 notes, music_tick stops the player. Subsequent SFX (sfx_correct, sfx_wrong) work normally on CH1.

---

## Phase 7 — Puzzle-select scene + replay flow

**Execution Status:** ⬜ NOT STARTED

**Goal**: New SCENE_PUZZLE_SELECT — visible from TITLE menu (only after at least 1 puzzle completed), shows a grid of all puzzles (50 cells), highlights completed ones in their hardest-tier color, lets player pick a completed puzzle to replay. Replay plays the chosen puzzle without advancing `current_puzzle_index`; on WIN, returns to puzzle-select rather than next puzzle.

**Why**: This is the biggest user-facing feature of v1.2. Combines with stats history (per-puzzle bests visible).

**Architecture**:
- New scene file: `src/game/scene_puzzle_select.c`
- New enum value: `SCENE_PUZZLE_SELECT`
- New scene_handoff: `extern uint8_t replay_puzzle_index;` (which puzzle is being replayed; sentinel 0xFF means "linear flow, not a replay")
- TITLE menu gains a new "PUZZLES" option (visible iff any bit in `completed_bits` is set)
- scene_play branches on `replay_puzzle_index`: if 0xFF, play `current_puzzle_index` (linear); else play `replay_puzzle_index` (replay mode, don't update current_puzzle_index on WIN)
- scene_win branches on `replay_puzzle_index`: if 0xFF, advance to next puzzle (current behavior); else transition to SCENE_PUZZLE_SELECT

This is the biggest phase — split into 4 tasks: scene file + grid render, input handling, scene_play replay branch, scene_win replay branch.

### Task 7.1: SCENE_PUZZLE_SELECT enum + scene_handoff additions + scene file scaffold

**Files:**
- Modify: `src/game/game_state.h` — add SCENE_PUZZLE_SELECT
- Modify: `src/game/scene_handoff.h` — add replay_puzzle_index extern
- Modify: `src/main.c` — define replay_puzzle_index global + add scene to vtable
- Create: `src/game/scene_puzzle_select.c` — scene scaffold
- Modify: `Makefile` — add scene_puzzle_select.c to SRC

- [ ] **Step 1: Add SCENE_PUZZLE_SELECT enum value**

In `src/game/game_state.h`, update Scene enum:

```c
typedef enum {
    SCENE_TITLE = 0,
    SCENE_PLAY,
    SCENE_WIN,
    SCENE_LOSE,
    SCENE_ALL_DONE,
    SCENE_PUZZLE_SELECT
} Scene;
```

- [ ] **Step 2: Add replay_puzzle_index to scene_handoff.h**

```c
// Set by SCENE_PUZZLE_SELECT before transitioning to SCENE_PLAY. Sentinel
// 0xFF means "linear flow, not a replay" — scene_play uses
// pg_save.current_puzzle_index as the puzzle to load. Any other value
// is the puzzle index to replay; scene_play uses that, and scene_win
// transitions back to SCENE_PUZZLE_SELECT on WIN instead of advancing
// to the next linear puzzle.
extern uint8_t replay_puzzle_index;
#define REPLAY_NONE 0xFF
```

- [ ] **Step 3: Define replay_puzzle_index in main.c**

In `src/main.c`, alongside `last_puzzle_result`:

```c
uint8_t replay_puzzle_index = REPLAY_NONE;
```

- [ ] **Step 4: Add scene to vtable in main.c**

Add the extern + table entry:

```c
extern const SceneVTable SCENE_PUZZLE_SELECT_VTABLE;

// Grow scene array from 5 to 6 entries
SceneVTable SCENES[6];

// In main, after the existing SCENES assignments:
SCENES[SCENE_PUZZLE_SELECT] = SCENE_PUZZLE_SELECT_VTABLE;
```

- [ ] **Step 5: Create the scene scaffold src/game/scene_puzzle_select.c**

```c
#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "scene_handoff.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include <gb/gb.h>
#include <stdio.h>
#include <stdbool.h>

static GameSave  ps_save;
static uint8_t   ps_cursor;        // 0..NUM_PUZZLES-1 — which puzzle the cursor is on
static bool      ps_redraw_needed;

// Grid: 5 columns wide, up to 13 rows. Each puzzle = 2 tiles wide × 2 tiles
// tall + 1 tile padding. With 50 puzzles: 10 rows × 5 cols. Grid origin:
// row 3 (leaves rows 0-2 for header), col 3 (centers).
#define GRID_COL_COUNT     5
#define GRID_CELL_W        3   // tile cell + padding per puzzle
#define GRID_CELL_H        2
#define GRID_ORIGIN_X      3
#define GRID_ORIGIN_Y      3

static void render_grid_cell(uint8_t puzzle_idx, bool is_cursor);
static void render_focused_stats(void);

static void ps_init(void) {
    BGP_REG = 0xE4;
    save_load(&ps_save);
    ps_cursor = 0;
    ps_redraw_needed = true;
}

static void ps_render(void) {
    if (!ps_redraw_needed) return;
    ps_redraw_needed = false;
    render_clear();

    render_text(5, 0, "PUZZLES");

    // Render the grid — one cell per puzzle 0..NUM_PUZZLES-1.
    for (uint8_t i = 0; i < NUM_PUZZLES; i++) {
        render_grid_cell(i, i == ps_cursor);
    }

    // Bottom: focused puzzle stats (puzzle number, best time, best tries)
    render_focused_stats();

    // Help text
    render_text(0, 17, "A PLAY  B BACK");
}

static void ps_update(Scene *next_scene) {
    uint8_t row = ps_cursor / GRID_COL_COUNT;
    uint8_t col = ps_cursor % GRID_COL_COUNT;

    // Cursor movement (d-pad). Wrap at row boundaries — moving past the
    // last puzzle in the grid wraps back to puzzle 0.
    if (input_repeat(BTN_LEFT)) {
        if (col > 0) {
            ps_cursor--;
        } else if (ps_cursor > 0) {
            ps_cursor--;  // wrap to last cell of previous row
        }
        ps_redraw_needed = true;
        sfx_move();
    }
    if (input_repeat(BTN_RIGHT)) {
        if (ps_cursor + 1 < NUM_PUZZLES) {
            ps_cursor++;
            ps_redraw_needed = true;
            sfx_move();
        }
    }
    if (input_repeat(BTN_UP)) {
        if (ps_cursor >= GRID_COL_COUNT) {
            ps_cursor = (uint8_t)(ps_cursor - GRID_COL_COUNT);
            ps_redraw_needed = true;
            sfx_move();
        }
    }
    if (input_repeat(BTN_DOWN)) {
        uint8_t target = (uint8_t)(ps_cursor + GRID_COL_COUNT);
        if (target < NUM_PUZZLES) {
            ps_cursor = target;
            ps_redraw_needed = true;
            sfx_move();
        }
    }

    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
        // Only allow playing completed puzzles. The current_puzzle_index
        // path lets the player play the linear-next puzzle from TITLE.
        bool is_completed = (ps_save.completed_bits[ps_cursor >> 3]
                          & (uint8_t)(1u << (ps_cursor & 7))) != 0;
        if (is_completed) {
            sfx_select();
            replay_puzzle_index = ps_cursor;
            *next_scene = SCENE_PLAY;
        } else {
            sfx_reject();
        }
    } else if (input_pressed(BTN_B)) {
        sfx_deselect();
        replay_puzzle_index = REPLAY_NONE;
        *next_scene = SCENE_TITLE;
    }
}

static void ps_teardown(void) {
}

// Helper implementations defined after the vtable to keep init/update/render
// scannable at top.

static void render_grid_cell(uint8_t puzzle_idx, bool is_cursor) {
    uint8_t row = puzzle_idx / GRID_COL_COUNT;
    uint8_t col = puzzle_idx % GRID_COL_COUNT;
    uint8_t x = (uint8_t)(GRID_ORIGIN_X + col * GRID_CELL_W);
    uint8_t y = (uint8_t)(GRID_ORIGIN_Y + row * GRID_CELL_H);

    bool is_completed = (ps_save.completed_bits[puzzle_idx >> 3]
                      & (uint8_t)(1u << (puzzle_idx & 7))) != 0;

    // Cell body: 2x2 tiles. Completed = UI_TILE_SOLID_DARK (dark fill).
    // Not completed = UI_TILE_FILL_LIGHT (light). Cursor = override
    // with UI_TILE_FILL_SEL (selected highlight).
    uint8_t body_tile;
    if (is_cursor) {
        body_tile = UI_TILE_FILL_SEL;
    } else if (is_completed) {
        body_tile = UI_TILE_SOLID_DARK;
    } else {
        body_tile = UI_TILE_FILL_LIGHT;
    }

    for (uint8_t dx = 0; dx < 2; dx++) {
        for (uint8_t dy = 0; dy < 2; dy++) {
            render_set_tile((uint8_t)(x + dx), (uint8_t)(y + dy), body_tile);
        }
    }

    // On GBC: color completed cells in a tier color (cycle through 4 tiers
    // by puzzle_idx % 4 — purely visual variety, not meaningful).
    if (is_completed) {
        uint8_t palette = (uint8_t)(GBC_PAL_TIER_YELLOW + (puzzle_idx % 4));
        for (uint8_t dx = 0; dx < 2; dx++) {
            for (uint8_t dy = 0; dy < 2; dy++) {
                render_set_tile_palette((uint8_t)(x + dx), (uint8_t)(y + dy), palette);
            }
        }
    }
}

static void render_focused_stats(void) {
    char buf[21];
    uint8_t p = ps_cursor;
    sprintf(buf, "PUZZLE %d", (int)(p + 1));
    render_text(2, 14, buf);

    bool is_completed = (ps_save.completed_bits[p >> 3]
                      & (uint8_t)(1u << (p & 7))) != 0;
    if (is_completed) {
        uint16_t mins = (uint16_t)(ps_save.puzzle_best_time[p] / 60);
        uint16_t secs = (uint16_t)(ps_save.puzzle_best_time[p] % 60);
        sprintf(buf, "BEST: %d:%02d  %d/4", (int)mins, (int)secs,
                (int)ps_save.puzzle_best_tries[p]);
        render_text(2, 15, buf);
    } else {
        render_text(2, 15, "NOT YET PLAYED");
    }
}

const SceneVTable SCENE_PUZZLE_SELECT_VTABLE = {
    .init = ps_init,
    .update = ps_update,
    .render = ps_render,
    .teardown = ps_teardown,
};
```

- [ ] **Step 6: Add to Makefile SRC**

```makefile
SRC := src/main.c \
       ... existing engine + game files ...
       src/game/scene_title.c src/game/scene_play.c src/game/scene_win.c \
       src/game/scene_lose.c src/game/scene_all_done.c \
       src/game/scene_puzzle_select.c \
       src/game/title_theme.c src/game/play_sfx.c
```

- [ ] **Step 7: Build**

```bash
make 2>&1 | tail -5
```

Expected: clean build. The new scene file may surface missing extern declarations or constant references — fix as they come up.

- [ ] **Step 8: Commit (scene scaffold; not yet reachable from TITLE)**

```bash
git add src/game/game_state.h src/game/scene_handoff.h src/main.c \
        src/game/scene_puzzle_select.c Makefile
git commit -m "v1.2.7.1: SCENE_PUZZLE_SELECT scaffold

New scene that's not yet reachable from TITLE (TITLE menu wiring lands
in 7.2). Grid view of 50 puzzles, completed cells colored by tier
(palette cycle 0..3 by puzzle_idx % 4), focused puzzle shows best
time/tries. A on a completed puzzle sets replay_puzzle_index +
transitions to SCENE_PLAY; B returns to TITLE.

Scene table grew from 5 to 6 entries. replay_puzzle_index global
(sentinel REPLAY_NONE = 0xFF means linear flow) added to scene_handoff."
```

### Task 7.2: Wire SCENE_PUZZLE_SELECT into TITLE menu

**Files:**
- Modify: `src/game/scene_title.c` — add "PUZZLES" menu option (visible iff any puzzle is completed)

- [ ] **Step 1: Add menu enum entry**

In scene_title.c, at the top, add a new TitleMenuItem:

```c
typedef enum {
    TITLE_MENU_CONTINUE = 0,
    TITLE_MENU_RESTART,
    TITLE_MENU_NEW_GAME,
    TITLE_MENU_PUZZLES,        // NEW v1.2 — visible iff any completed_bits set
    TITLE_MENU_COUNT
} TitleMenuItem;
```

- [ ] **Step 2: Add visibility flag + menu count logic**

In TitleState:

```c
typedef struct {
    GameSave save;
    bool     has_in_progress;
    bool     has_any_progress;
    bool     has_any_completed;       // NEW v1.2
    uint8_t  menu_cursor;
    uint8_t  menu_item_count;
    bool     show_confirm;
    bool     redraw_needed;
} TitleState;
```

In `title_init`, set `has_any_completed`:

```c
    ts.has_any_completed = false;
    for (uint8_t i = 0; i < MAX_PUZZLES_SUPPORTED / 8; i++) {
        if (ts.save.completed_bits[i] != 0) {
            ts.has_any_completed = true;
            break;
        }
    }
```

Update `menu_item_count`: bump by 1 when puzzles available:

```c
    if (ts.has_in_progress) {
        ts.menu_item_count = 3;   // CONTINUE / RESTART / NEW GAME
    } else if (ts.has_any_progress) {
        ts.menu_item_count = 2;   // CONTINUE / NEW GAME
    } else {
        ts.menu_item_count = 1;   // NEW GAME only
    }
    if (ts.has_any_completed) {
        ts.menu_item_count++;     // + PUZZLES at the bottom
    }
```

- [ ] **Step 3: Render PUZZLES menu line + handle selection**

In `title_render`, render the PUZZLES option after the existing menu items (at the appropriate row). Use the same `>` cursor convention. The trick: the cursor index for PUZZLES depends on what other items are visible.

If `has_in_progress`: 3-item base menu (CONTINUE, RESTART, NEW GAME). PUZZLES is at cursor 3 (4th item). Row = base_row + 3.
If `has_any_progress` but no in-progress: 2-item base menu. PUZZLES at cursor 2. Row = base_row + 2.
If fresh save: 1-item base menu (NEW GAME only). But PUZZLES requires `has_any_completed` which requires a completed puzzle... so this case can't happen (a completed puzzle implies has_any_progress).

Render block in title_render:

```c
    // Render PUZZLES menu item if visible. Its row depends on how many
    // other items are visible above it.
    if (ts.has_any_completed) {
        uint8_t puzzles_cursor_idx;
        uint8_t puzzles_row;
        if (ts.has_in_progress) {
            puzzles_cursor_idx = 3;
            puzzles_row = row + 3;
        } else {
            puzzles_cursor_idx = 2;
            puzzles_row = row + 2;
        }
        buf[0] = (ts.menu_cursor == puzzles_cursor_idx) ? '>' : ' ';
        buf[1] = 'P'; buf[2] = 'U'; buf[3] = 'Z'; buf[4] = 'Z'; buf[5] = 'L';
        buf[6] = 'E'; buf[7] = 'S'; buf[8] = 0;
        render_text(2, puzzles_row, buf);
    }
```

In `title_update`'s selection handler, add a branch for PUZZLES:

```c
    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
        // ... existing branches for CONTINUE/RESTART/NEW GAME ...

        // After all other branches, check if cursor is on PUZZLES item.
        uint8_t puzzles_cursor_idx;
        if (ts.has_in_progress) {
            puzzles_cursor_idx = 3;
        } else {
            puzzles_cursor_idx = 2;
        }
        if (ts.has_any_completed && ts.menu_cursor == puzzles_cursor_idx) {
            sfx_select();
            *next_scene = SCENE_PUZZLE_SELECT;
            return;
        }
    }
```

This integrates with the existing branched logic that handles other menu items. The exact integration depends on the existing structure — read the current title_update before editing.

- [ ] **Step 4: Build + verify in mGBA**

```bash
make 2>&1 | tail -3
```

Test flow:
1. Fresh save (no completed puzzles): TITLE shows only "NEW GAME" — no PUZZLES option (correct).
2. Complete a puzzle, return to TITLE: PUZZLES option now visible.
3. Cursor down to PUZZLES, press A: transitions to SCENE_PUZZLE_SELECT.
4. In puzzle-select: cursor over puzzle 1 (completed) shows "BEST: x:xx 1/4" or similar; press A → SCENE_PLAY launches puzzle 1 again.
5. In puzzle-select: cursor over puzzle 2 (not completed) shows "NOT YET PLAYED"; press A → sfx_reject (rejection sound, no transition).
6. Press B from puzzle-select → back to TITLE.

- [ ] **Step 5: Commit**

```bash
git add src/game/scene_title.c
git commit -m "v1.2.7.2: TITLE menu — PUZZLES option (visible after first completion)

PUZZLES menu item appears below the existing CONTINUE/RESTART/NEW GAME
items iff any bit in completed_bits is set. Selecting it transitions
to SCENE_PUZZLE_SELECT. Menu count grows dynamically: +1 row when
any completed puzzle exists."
```

### Task 7.3: scene_play replay branch + scene_win replay return

**Files:**
- Modify: `src/game/scene_play.c` — when replay_puzzle_index != REPLAY_NONE, load that puzzle
- Modify: `src/game/scene_win.c` — when replay_puzzle_index != REPLAY_NONE, return to SCENE_PUZZLE_SELECT instead of next puzzle

- [ ] **Step 1: scene_play — branch on replay_puzzle_index**

In scene_play.c's `play_init`, after save_load:

```c
    // v1.2 replay mode: if replay_puzzle_index is set, use it instead of
    // pg_save.current_puzzle_index. Replay mode also forces a fresh
    // puzzle state (no in-progress restoration) — replaying is always
    // starting fresh on that specific puzzle.
    uint8_t puzzle_to_load;
    if (replay_puzzle_index != REPLAY_NONE) {
        puzzle_to_load = replay_puzzle_index;
        // Force fresh state (no ip_* carryover from a different puzzle).
        ps.tries_remaining = 4;
        ps.groups_solved   = 0;
        ps.selected_mask   = 0;
        ps.elapsed_seconds = 0;
    } else {
        puzzle_to_load = pg_save.current_puzzle_index;
        // Existing in-progress restoration logic continues here.
        if (pg_save.ip_tries_remaining > 0) {
            ps.tries_remaining = pg_save.ip_tries_remaining;
            ps.groups_solved   = pg_save.ip_groups_solved;
            ps.selected_mask   = pg_save.ip_selected_mask;
            ps.elapsed_seconds = pg_save.ip_elapsed_seconds;
        } else {
            ps.tries_remaining = 4;
            ps.groups_solved   = 0;
            ps.selected_mask   = 0;
            ps.elapsed_seconds = 0;
        }
    }
    ps.cursor_idx = 0;
    ps.show_quit_confirm = 0;
```

Wherever scene_play references `pg_save.current_puzzle_index` for the active puzzle (loading words, rendering header), substitute `puzzle_to_load`. This is a non-trivial refactor — scan the file for all `pg_save.current_puzzle_index` references and replace them with the local `puzzle_to_load` variable (which is captured in `play_init` and stored in a new `static uint8_t play_puzzle_idx;` for use across play_update / play_render).

```c
static uint8_t play_puzzle_idx;  // which puzzle this PLAY session loaded; set in play_init
```

In play_init: `play_puzzle_idx = puzzle_to_load;`
Everywhere else: replace `pg_save.current_puzzle_index` with `play_puzzle_idx`.

CRITICAL: when scene_play handles the WIN transition, do NOT advance pg_save.current_puzzle_index if we're in replay mode (it would corrupt the linear progression). Wrap the existing advance in a check:

```c
                if (replay_puzzle_index == REPLAY_NONE) {
                    pg_save.current_puzzle_index++;
                }
```

But the completed_bits + per-puzzle records updates SHOULD fire (replay can set new bests). Keep those updates unconditional but reference `play_puzzle_idx` instead of `pg_save.current_puzzle_index`:

```c
                {
                    uint8_t puzzle_idx = play_puzzle_idx;  // not pg_save.current_puzzle_index
                    ... rest of best-records update ...
                }
```

- [ ] **Step 2: scene_win — return to puzzle-select on replay WIN**

In scene_win's `win_update`, the START input handler currently transitions to SCENE_PLAY or SCENE_ALL_DONE. Add a check:

```c
    if (input_pressed(BTN_START)) {
        sfx_select();
        if (replay_puzzle_index != REPLAY_NONE) {
            // Replay mode — clear the replay flag and return to puzzle-select.
            replay_puzzle_index = REPLAY_NONE;
            *next_scene = SCENE_PUZZLE_SELECT;
        } else if (win_save.current_puzzle_index >= NUM_PUZZLES) {
            *next_scene = SCENE_ALL_DONE;
        } else {
            *next_scene = SCENE_PLAY;
        }
    } else if (input_pressed(BTN_SELECT)) {
        sfx_deselect();
        replay_puzzle_index = REPLAY_NONE;  // also clear on SELECT-quit
        *next_scene = SCENE_TITLE;
    }
```

- [ ] **Step 3: scene_lose — also clear replay flag on exit**

Similar: in scene_lose's input handlers (RETRY / SKIP / SELECT-quit), if `replay_puzzle_index != REPLAY_NONE`, return to SCENE_PUZZLE_SELECT instead of SCENE_PLAY (RETRY) or normal progression. Or simpler: any exit from scene_lose during a replay returns to puzzle-select.

Actually losing during a replay is unusual — most replays are by players trying to beat their record. Let me handle it minimally: any input that exits scene_lose during a replay returns to puzzle-select. RETRY-during-replay restarts the same puzzle (replay flag stays set), SKIP-during-replay returns to puzzle-select (replay flag cleared).

```c
    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
        if (lose_menu_cursor == 0) {
            // RETRY — restart same puzzle. If replay, stays in replay mode.
            sfx_select();
            *next_scene = SCENE_PLAY;
        } else {
            // SKIP. If replay, just return to puzzle-select (don't advance
            // anything; replay is a discretionary action).
            if (replay_puzzle_index != REPLAY_NONE) {
                sfx_skip();
                replay_puzzle_index = REPLAY_NONE;
                *next_scene = SCENE_PUZZLE_SELECT;
            } else {
                // existing SKIP logic — advance, save, transition to PLAY
                ... existing block ...
            }
        }
    } else if (input_pressed(BTN_SELECT)) {
        sfx_deselect();
        replay_puzzle_index = REPLAY_NONE;
        *next_scene = SCENE_TITLE;
    }
```

- [ ] **Step 4: Build + verify in mGBA**

```bash
make 2>&1 | tail -3
```

Test flow:
1. Complete puzzles 1, 2, 3 (linear).
2. TITLE → PUZZLES → cursor to puzzle 1 → A. PLAY loads puzzle 1.
3. Solve puzzle 1 (faster than before). WIN scene shows stats; START returns to PUZZLE_SELECT (NOT to puzzle 2!).
4. Back in PUZZLE_SELECT: focused stats for puzzle 1 should now show the NEW best time/tries.
5. B → TITLE. The TITLE's CONTINUE should still point to your original linear progression (puzzle 4, not back to 2).

- [ ] **Step 5: Commit**

```bash
git add src/game/scene_play.c src/game/scene_win.c src/game/scene_lose.c
git commit -m "v1.2.7.3: scene_play replay mode + scene_win/lose return to PUZZLE_SELECT

scene_play loads play_puzzle_idx (= replay_puzzle_index if set, else
current_puzzle_index). Replay mode: fresh state (no in-progress carry),
current_puzzle_index NOT advanced on WIN, per-puzzle records still
update (replay can beat your record).

scene_win: START returns to PUZZLE_SELECT during replay (instead of
next linear puzzle). scene_lose: SKIP returns to PUZZLE_SELECT; RETRY
restarts same puzzle in replay mode."
```

### Phase 7 group review

1. **Linear flow preserved**: after a replay, linear CONTINUE still points to wherever you left off.
2. **Replay updates records**: beating your old best time on a replayed puzzle updates puzzle_best_time correctly.
3. **Migration interop**: legacy v1 players who migrate get all puzzles 0..(current_puzzle_index-1) marked complete; can immediately replay any of those via PUZZLE_SELECT (will show "—" / no record until they set one).
4. **No regression in linear flow**: NEW GAME → solve → WIN → next puzzle still works.

---

## Phase 8 — Smaller ALL_DONE polish

**Execution Status:** ⬜ NOT STARTED

**Goal**: ALL_DONE scene gets a longer chiptune flourish + scrolling "thanks for playing" text. No big animation. End state: hitting ALL_DONE feels like a genuine milestone (audio is the payoff).

**Approach**: Define ALL_DONE_FANFARE — a 12-16 step MusicTrack on CH1 (+ optional CH2 harmony). Play it once in scene_all_done's init. Scroll the existing "YOU SOLVED ALL N!" text horizontally one tile per ~10 frames so it has motion. Keep the existing menu structure.

### Task 8.1: Author ALL_DONE_FANFARE + add scrolling text

**Files:**
- Create: `src/game/all_done_theme.c` + `src/game/all_done_theme.h`
- Modify: `src/game/scene_all_done.c` — play fanfare on init, scroll header text

- [ ] **Step 1: Create all_done_theme.c**

```c
#include "all_done_theme.h"

// ~10 seconds of celebration. Builds from a 4-note opening to a sustained
// final chord (C major triad on CH1 + CH2). Non-looping (player gets
// silence once it finishes — the silence is also part of the moment).
static const MusicStep ALL_DONE_FANFARE_STEPS[16] = {
    // Opening: ascending fanfare (1.5s)
    { MUSIC_NOTE_C5, 12 }, { MUSIC_NOTE_E5, 12 }, { MUSIC_NOTE_G5, 12 }, { MUSIC_NOTE_C6, 24 },
    // Echoing descent (1s)
    { MUSIC_NOTE_G5, 12 }, { MUSIC_NOTE_E5, 12 }, { MUSIC_NOTE_C5, 24 },
    // Sustained melody — slower, savoring (3s)
    { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_C6, 24 },
    { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_C5, 24 },
    // Final ascending resolution (~3s)
    { MUSIC_NOTE_C5, 12 }, { MUSIC_NOTE_E5, 12 }, { MUSIC_NOTE_G5, 60 },  // hold final
};

static const uint8_t ALL_DONE_FANFARE_CH2[16] = {
    MUSIC_NOTE_G4, MUSIC_NOTE_C5, MUSIC_NOTE_E5, MUSIC_NOTE_G5,
    MUSIC_NOTE_E5, MUSIC_NOTE_C5, MUSIC_NOTE_G4,
    MUSIC_NOTE_C5, MUSIC_NOTE_E5, MUSIC_NOTE_G5,
    MUSIC_NOTE_E5, MUSIC_NOTE_C5, MUSIC_NOTE_G4,
    MUSIC_NOTE_G4, MUSIC_NOTE_C5, MUSIC_NOTE_E5,
};

const MusicTrack ALL_DONE_FANFARE = {
    .steps      = ALL_DONE_FANFARE_STEPS,
    .ch2_notes  = ALL_DONE_FANFARE_CH2,
    .length     = 16,
    .loop_start = 16,  // non-looping
};
```

Create the header at `src/game/all_done_theme.h`:

```c
#ifndef GAME_ALL_DONE_THEME_H
#define GAME_ALL_DONE_THEME_H
#include "../engine/music.h"
extern const MusicTrack ALL_DONE_FANFARE;
#endif
```

- [ ] **Step 2: Scroll the header text in scene_all_done**

In scene_all_done.c, add scroll-position state:

```c
static uint8_t scroll_offset;       // 0..19 — horizontal shift of the YOU SOLVED text
static uint16_t scroll_last_frame;
extern volatile uint16_t global_frame_count;
```

In init: `scroll_offset = 0; scroll_last_frame = global_frame_count; music_play(&ALL_DONE_FANFARE);`

In update: every 10 frames, increment scroll_offset (with wrap):

```c
    if ((uint16_t)(global_frame_count - scroll_last_frame) >= 10) {
        scroll_offset = (uint8_t)((scroll_offset + 1) % 20);
        scroll_last_frame = (uint16_t)(scroll_last_frame + 10);
        // mark render dirty so render redraws
    }
```

In render: render the "YOU SOLVED ALL N!" text at column `(scroll_offset)` instead of fixed position. The text wraps off-screen on the right and reappears on the left (uses two render_text calls when wrapping).

This is a small enough change that the executor can implement it directly. Specifics:

```c
    char header[21];
    sprintf(header, "YOU SOLVED ALL %d!", (int)NUM_PUZZLES);
    // Pad header to 20 chars for clean scroll (uses spaces).
    uint8_t header_len = 0;
    while (header[header_len] && header_len < 20) header_len++;
    // Scroll position: render starts at column scroll_offset, wraps around.
    for (uint8_t i = 0; i < header_len; i++) {
        uint8_t col = (uint8_t)((scroll_offset + i) % 20);
        char glyph[2] = { header[i], 0 };
        render_text(col, 5, glyph);  // row 5 = title area
    }
```

- [ ] **Step 3: Add all_done_theme.c to Makefile**

```makefile
SRC := ... \
       src/game/all_done_theme.c
```

- [ ] **Step 4: Build + verify in mGBA**

```bash
make 2>&1 | tail -3
```

Trigger ALL_DONE by completing all 30 (or 50) puzzles, OR by using mGBA's memory editor to set `current_puzzle_index = NUM_PUZZLES` and winning a puzzle. ALL_DONE scene should:
- Render header text that visibly scrolls horizontally
- Play the 10-second 2-voice fanfare
- After fanfare ends, screen + menu work normally (START → TITLE)

- [ ] **Step 5: Commit**

```bash
git add src/game/all_done_theme.h src/game/all_done_theme.c \
        src/game/scene_all_done.c Makefile
git commit -m "v1.2.8: ALL_DONE polish — scrolling header + 10s fanfare

Header text scrolls horizontally (1 col every 10 frames, wraps).
ALL_DONE_FANFARE: 16-step CH1 melody + 16-step CH2 harmony, ~10s
total, non-looping. Built on the music engine's CH2 support from
Phase 4. No big animation — audio is the payoff."
```

### Phase 8 group review

1. **Fanfare plays once cleanly**: doesn't loop, doesn't restart on every menu navigation, gracefully silent after completion.
2. **Scroll doesn't flicker**: render_clear in scene_all_done's full-redraw path doesn't cause visible mid-render artifacts.
3. **Menu still works**: START returns to TITLE (cycle restart per existing behavior).

---

## Phase 9 — Content: +20 puzzles (to 50 total) + v1.2 release

**Execution Status:** ⬜ NOT STARTED

**Goal**: Author 20 more puzzles (IDs 31-50) and tag v1.2.

### Task 9.1: Author 20 puzzles (3 batches if preferred, or 1 commit)

**Files:**
- Modify: `content/puzzles.json` — add 20 entries

The executor (or agent) authors puzzles 31-50 following the same simple-classification style as 1-30. Constraints unchanged from Plan C:
- Sequential IDs starting at 31
- 4 categories per puzzle (yellow/green/blue/purple)
- 4 words each (uppercase A-Z, 1-8 chars)
- Category names ≤12 chars uppercase
- No within-puzzle duplicates
- No name = word collision

- [ ] **Step 1: Author puzzles 31-40 (or any 10-puzzle batch)**

Pick 10 themes that haven't been used (or fresh angles on used themes). Example themes that haven't been deeply explored: video games / pop culture, weather phenomena, building types, dance styles, gym/fitness, world capitals beyond the obvious, kitchen appliances, beverages by region, board games, etc.

Append the puzzle objects to `content/puzzles.json`. Run `make test` to validate, `make` to confirm clean build, then commit:

```bash
git add content/puzzles.json
git commit -m "v1.2.9.1: author puzzles 31-40"
```

- [ ] **Step 2: Author puzzles 41-50**

Same pattern. Append, validate, commit:

```bash
git add content/puzzles.json
git commit -m "v1.2.9.2: author puzzles 41-50 (50-puzzle bank complete)"
```

- [ ] **Step 3: Full test pass + ROM stats**

```bash
make clean && make && make test
ls -la build/gameboygame.gb
python -c "data = open('build/gameboygame.gb','rb').read(); ff = data.count(b'\xff'); print(f'ROM: {len(data)} bytes, {100*(len(data)-ff)/len(data):.1f}% used')"
```

Expected: ROM still 65,536 bytes (bank-padded). Usage up by ~5KB from 20 new puzzles. All tests pass.

### Task 9.2: Update README + tag v1.2 + release

**Files:**
- Modify: `README.md` — bump version mentions

- [ ] **Step 1: README updates**

Update the project status line:
```markdown
**v1.2 shipped 2026-05-26.** Adds puzzle-select replay screen, per-puzzle best records, tiered milestone celebrations, multi-voice title music, CH1 fanfares on puzzle start + ALL_DONE, slide-in bar animations, and 20 more puzzles (50 total).
```

Note that "Each puzzle has 16 words..." stays the same. Update the puzzle count from 30 to 50 in the project description.

- [ ] **Step 2: Tag v1.2**

```bash
git tag -a v1.2 -m "Game Boy Connections v1.2 — Replay & Polish

Major additions:
  - Puzzle-select scene: replay any completed puzzle from a grid view
  - Per-puzzle records (best time, fewest tries) persisted across sessions
  - Tiered milestone celebrations: small fanfare every 5 lifetime solves,
    bigger fanfare every 5 streak puzzles
  - Multi-voice music: title theme now plays CH1 melody + CH2 harmony
  - Puzzle-start CH1 stinger (4-note fanfare on fresh puzzle)
  - ALL_DONE: 10-second 2-voice fanfare + scrolling header text
  - Cascade bars slide in from the left over 5 frames (scene_win + scene_lose)
  - Content: 20 more puzzles (30 → 50)

Save migration:
  - v1.0/v1.1 saves are auto-migrated on first v1.2 boot
  - Linear progression backfills completed_bits (puzzles 0..N-1 marked complete)
  - Per-puzzle records start blank for migrated saves (replay sets new bests)

Same ROM works on both DMG and GBC. Backwards-compatible save format
(legacy saves preserved)."
```

- [ ] **Step 3: Push + create GitHub release**

```bash
git push origin main
git push origin v1.2
gh release create v1.2 build/gameboygame.gb --title "Game Boy Connections v1.2" --notes-from-tag
```

- [ ] **Step 4: Update plan banners to ✅ SHIPPED**

Mark all phases shipped in the top-of-plan execution table + each per-phase banner. Add celebration:

```markdown
🎉 **Plan complete — v1.2 tagged and released.**
```

Commit + push the plan update.

### Phase 9 group review

1. **Clean ROM build with 50 puzzles**: no NUM_PUZZLES overflow, codegen runs cleanly, ROM still fits in 64KB.
2. **Save migration tested end-to-end**: an actual v1.1 save migrates cleanly on first v1.2 boot, all UI elements (puzzle-select grid, TITLE menu) reflect the migrated state.
3. **Release published**: GitHub release page shows v1.2 with the new ROM asset; CHANGELOG-equivalent (tag annotation) describes the additions.

---

## End of Plan

When all 9 phases ship and their banners are updated, v1.2 is released. The artifact:

- 50-puzzle bank with replay support
- Save migration preserving v1.0/v1.1 player progress
- Multi-voice title music + puzzle-start stinger + ALL_DONE fanfare
- Cascade-bar slide-in animations
- Tiered celebrations at every-5 milestones (lifetime + streak)
- Same ROM works on DMG and GBC (unchanged from v1.1)
- All host tests passing (74 → 76 with 2 new CH2 tests)

**Deferred to v1.3+:**
- Banked puzzle data (would unlock 100+ puzzles)
- Hardware verification on real DMG via flash cart
- Hint system (mentioned in Plan D survey)
- Daily-puzzle mode (user declined due to RTC complexity)
- Multi-channel SFX (current SFX still single-channel; would unlock richer audio events like layered correct-chime)
- LSDJ tracker integration (out of scope per user preference)
