# Game Boy Connections — Gameplay Plan (Plan B of 3)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the gameplay layer on top of Plan A's foundation — the 5 scenes (TITLE, PLAY, WIN, LOSE, ALL_DONE) that turn the smoke-test scaffolding into a real playable Connections game. End state is a `.gb` ROM where the player can boot to title, start a new game, play through any of the 5 sample puzzles, win or lose, see stats, and have all state persist across power cycles.

**Architecture:** Continue Plan A's engine/game split. New game code lives in `src/game/scene_*.c` and `src/game/layout.c` — pure C with no GBDK includes (calls only into `src/engine/*` APIs Plan A delivered). `src/main.c` becomes a thin scene dispatcher with a VBlank ISR that drives `anim_tick`, `sound_tick`, and `render_flush`. Each scene exposes an `init / update / render / teardown` quartet, dispatched via a function-pointer table indexed by `Scene` enum (from `game_state.h`). Plan A's 7 animation stubs (SELECT_FLASH, CELL_FLASH, CORRECT_FLASH, LAYOUT_REFLOW, BAR_CASCADE, STATS_FADE, LOSE_REVEAL) get real implementations alongside the scenes that use them.

**Tech Stack:** Same as Plan A — GBDK-2020 (lcc + SDCC, DMG target), MinGW64 gcc 16.1.0 (host-side tests), Python 3 stdlib (asset codegen, no third-party deps), GNU Make. Emulator: mGBA on Windows.

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

## Notes for executors

**Inherited from Plan A.** Plan B builds on top of Plan A (`docs/plans/2026-05-24-gameboygame-foundation-plan.md`, shipped 2026-05-24 at SHAs `d98081c..2cca577`). Plan A's Notes for executors and Discoveries section still apply — in particular:
- All `Bash` tool calls must prepend `export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:/c/gbdk/bin:$PATH"` to see the GBDK, MinGW64 gcc, and MSYS2 make.
- `test/Makefile` needs `TMPDIR=/tmp` exports (already in place).
- Top-level `Makefile` has `.DEFAULT_GOAL := all`.
- Emulator is mGBA on Windows; verification techniques translate to BGB/SameBoy.

**TDD scope.** Plan B has fewer host-testable units than Plan A — scene logic is GBDK-touching (renders to tilemap buffer, reads input via `joypad()`, schedules animations via engine APIs). The one new pure-function module is `src/game/layout.c` (dynamic compacting layout calculator), which IS host-testable and DOES require TDD. The five scene files are verified via mGBA smoke tests, the same pattern Plan A used for engine subsystems.

**Verifying behavior in mGBA.** Plan B scenes are interactive — verification means actually playing through scene transitions and observing the result. The plan structures each scene phase so the user can verify it in mGBA before the next scene depends on it. Use `make` to rebuild after edits; load `build/gameboygame.gb` in mGBA via File → Load ROM.

**Save corruption during dev.** Each Plan B scene touches the SRAM save more aggressively than Plan A. mGBA persists SRAM to `build/gameboygame.sav` next to the ROM. If a scene gets a bad save state during dev and won't recover (e.g., `current_puzzle_index` out of bounds), delete the `.sav` file to force a `save_reset()` on next boot.

**Commit cadence.** Continue Plan A's pattern: one commit per phase with subject `<phase>.<task>: <imperative summary>` (the first commit of the phase usually carries the phase name in the subject). One banner-update commit per phase follows the work commit.

**Pitfalls docs status.** Still none. Plan A's "Notes for executors" deferred creating `docs/pitfalls/implementation-pitfalls.md` and `docs/pitfalls/testing-pitfalls.md` until real GB-specific traps surfaced. Plan B's execution will likely surface several (timing-sensitive sprite OAM updates, palette swap race conditions, save-write-during-animation issues). A future plan can introduce the pitfalls docs once we've accumulated enough learned-the-hard-way knowledge.

---

## Execution Status

**Overall:** 0/9 phases shipped, 0 deferred.

| Phase | Status | Ship SHA(s) | Notes |
|---|---|---|---|
| 1 — Scene dispatch infrastructure | ⬜ Not started | — | thin main.c + scene table + VBlank ISR + stub scenes |
| 2 — TDD compute_play_layout() | ⬜ Not started | — | pure function, host-testable |
| 3 — UI tile assets + title art | ⬜ Not started | — | ui_tiles.png (cursor, borders, 4 tier patterns) + title.png |
| 4 — SCENE_TITLE | ⬜ Not started | — | adaptive menu + NEW GAME confirm + save writes |
| 5 — SCENE_PLAY rendering + cursor | ⬜ Not started | — | grid + cursor sprite + nav + SELECT_FLASH |
| 6 — SCENE_PLAY submission + animations | ⬜ Not started | — | START/B logic + CELL_FLASH + CORRECT_FLASH + LAYOUT_REFLOW + transitions |
| 7 — SCENE_WIN | ⬜ Not started | — | BAR_CASCADE + STATS_FADE + next-puzzle/title flow |
| 8 — SCENE_LOSE | ⬜ Not started | — | LOSE_REVEAL + retry/skip menu (skip gated on fails≥3) |
| 9 — SCENE_ALL_DONE + size check | ⬜ Not started | — | lifetime totals + cycle restart + final ROM size verification |

### Deviations
*(none yet — record per the Living Document Contract as they occur)*

### Discoveries
*(none yet — record per the Living Document Contract as they occur)*

---

## Phase 1 — Scene dispatch infrastructure

**Execution Status:** ⬜ NOT STARTED

**Goal**: Replace Plan A's smoke-test `main.c` with a clean scene dispatcher driven by a function-pointer table. Add a VBlank ISR that runs `anim_tick`, `sound_tick`, and `render_flush` once per frame. Provide stub `init/update/render/teardown` functions for all 5 scenes that just display the scene name and switch on START. End state: in mGBA, the user can cycle through all 5 scenes by pressing START, confirming the dispatcher works before any scene has real logic.

### Task 1.1: Define scene.h API

**Files:**
- Create: `src/game/scene.h`

- [ ] **Step 1: Write the scene interface header**

```c
// src/game/scene.h
#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include "game_state.h"

// Each scene exposes 4 lifecycle hooks. main.c's dispatcher calls them
// in this order each frame:
//   1. If transitioning into this scene, call init() once.
//   2. update() — read input, mutate scene-internal state, schedule
//      animations, trigger transitions by setting *next_scene.
//   3. render() — write to the tilemap buffer (via engine/render).
//      MUST NOT touch VRAM directly; engine/render flushes during VBlank.
//   4. If transitioning out, call teardown() once.
//
// Scenes set *next_scene from inside update() to request a transition.
// next_scene == current_scene means "stay". The dispatcher in main.c
// reads *next_scene after update() runs.

typedef struct {
    void (*init)(void);
    void (*update)(Scene *next_scene);
    void (*render)(void);
    void (*teardown)(void);
} SceneVTable;

// Scene table indexed by Scene enum from game_state.h. Defined in main.c.
extern const SceneVTable SCENES[];

#endif
```

### Task 1.2: Stub all 5 scene files

**Files:**
- Create: `src/game/scene_title.c`
- Create: `src/game/scene_play.c`
- Create: `src/game/scene_win.c`
- Create: `src/game/scene_lose.c`
- Create: `src/game/scene_all_done.c`

Each stub is identical except for the display string and the next scene transition. Write all 5 with the same pattern — START advances to the next scene in a cycle (TITLE → PLAY → WIN → LOSE → ALL_DONE → TITLE).

- [ ] **Step 1: Write scene_title.c stub**

```c
// src/game/scene_title.c
#include "scene.h"
#include "../engine/render.h"
#include "../engine/input.h"

static void title_init(void) {
    render_clear();
    render_text(2, 4, "SCENE TITLE");
    render_text(2, 8, "START -> PLAY");
}

static void title_update(Scene *next_scene) {
    if (input_pressed(BTN_START)) *next_scene = SCENE_PLAY;
}

static void title_render(void) {
    // Stub: init() draws everything once; no per-frame redraw needed.
}

static void title_teardown(void) {
    // Nothing to clean up in the stub.
}

const SceneVTable SCENE_TITLE_VTABLE = {
    .init = title_init,
    .update = title_update,
    .render = title_render,
    .teardown = title_teardown,
};
```

- [ ] **Step 2: Write scene_play.c stub (transitions to WIN on START)**

Same pattern as scene_title.c, but with "SCENE PLAY", "START -> WIN", and transitions to `SCENE_WIN`. Export `SCENE_PLAY_VTABLE`.

- [ ] **Step 3: Write scene_win.c stub (transitions to LOSE on START)**

Same pattern. "SCENE WIN", "START -> LOSE", transitions to `SCENE_LOSE`. Export `SCENE_WIN_VTABLE`.

- [ ] **Step 4: Write scene_lose.c stub (transitions to ALL_DONE on START)**

Same pattern. "SCENE LOSE", "START -> ALL DONE", transitions to `SCENE_ALL_DONE`. Export `SCENE_LOSE_VTABLE`.

- [ ] **Step 5: Write scene_all_done.c stub (transitions to TITLE on START)**

Same pattern. "SCENE ALL DONE", "START -> TITLE", transitions to `SCENE_TITLE`. Export `SCENE_ALL_DONE_VTABLE`.

### Task 1.3: Rewrite main.c as scene dispatcher with VBlank ISR

**Files:**
- Modify: `src/main.c` (full rewrite — current contents are Plan A's puzzle-display smoke test)

- [ ] **Step 1: Write the scene table + ISR + main loop**

```c
// src/main.c
#include <gb/gb.h>
#include "engine/render.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/sound.h"
#include "engine/anim.h"
#include "game/scene.h"
#include "game/game_state.h"

// External vtable declarations from each scene_*.c
extern const SceneVTable SCENE_TITLE_VTABLE;
extern const SceneVTable SCENE_PLAY_VTABLE;
extern const SceneVTable SCENE_WIN_VTABLE;
extern const SceneVTable SCENE_LOSE_VTABLE;
extern const SceneVTable SCENE_ALL_DONE_VTABLE;

const SceneVTable SCENES[] = {
    [SCENE_TITLE]    = { 0 },  // populated below in main()
    [SCENE_PLAY]     = { 0 },
    [SCENE_WIN]      = { 0 },
    [SCENE_LOSE]     = { 0 },
    [SCENE_ALL_DONE] = { 0 },
};

// Workaround: GBDK SDCC's initialization of designated-initializer
// arrays of structs sometimes fails to copy nested function pointers
// correctly when the structs come from other translation units. So
// we populate SCENES[] imperatively in main() instead of using the
// designated-initializer form above. (If a future GBDK release fixes
// this, the designated initializers above can replace the imperative
// assignment.)

// VBlank ISR — called by GBDK once per frame (60 Hz on DMG).
// Runs the per-frame engine-side ticks. Keep this lean: VBlank window
// is short (~1.1ms), and overrunning corrupts the next frame's rendering.
static void vblank_isr(void) {
    anim_tick();
    sound_tick();
    render_flush();
}

void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();

    // Populate scene table (see workaround comment above)
    ((SceneVTable *)SCENES)[SCENE_TITLE]    = SCENE_TITLE_VTABLE;
    ((SceneVTable *)SCENES)[SCENE_PLAY]     = SCENE_PLAY_VTABLE;
    ((SceneVTable *)SCENES)[SCENE_WIN]      = SCENE_WIN_VTABLE;
    ((SceneVTable *)SCENES)[SCENE_LOSE]     = SCENE_LOSE_VTABLE;
    ((SceneVTable *)SCENES)[SCENE_ALL_DONE] = SCENE_ALL_DONE_VTABLE;

    add_VBL(vblank_isr);

    Scene current = SCENE_TITLE;
    SCENES[current].init();

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    while (1) {
        input_update();

        Scene next = current;
        SCENES[current].update(&next);
        SCENES[current].render();

        if (next != current) {
            SCENES[current].teardown();
            current = next;
            SCENES[current].init();
        }

        wait_vbl_done();
    }
}
```

**About `add_VBL`:** GBDK's `add_VBL(fn)` registers a function to run from the VBlank interrupt handler. This is how engine ticks get a stable 60Hz heartbeat. Multiple handlers can be registered; they run in registration order.

**About `SHOW_SPRITES`:** Plan A's smoke tests didn't use sprites; Plan B does (cursor in PLAY scene). Enabling sprites at boot is harmless if no sprite tiles are configured yet — they just render as blank.

### Task 1.4: Add new files to Makefile SRC

**Files:**
- Modify: `Makefile`

- [ ] **Step 1: Update the SRC list**

Replace the existing SRC line with:

```makefile
SRC := src/main.c \
       src/engine/render.c src/engine/input.c src/engine/save.c \
       src/engine/sound.c src/engine/anim.c \
       src/game/puzzle_logic.c \
       src/game/scene_title.c src/game/scene_play.c src/game/scene_win.c \
       src/game/scene_lose.c src/game/scene_all_done.c
SRC += $(ASSETS_GEN)
SRC += $(PUZZLE_OUT)
```

Note: this is the first time `src/game/puzzle_logic.c` joins the ROM build — Plan A only used it for host-side tests.

### Task 1.5: Build and verify scene cycling in mGBA

**Files:** none

- [ ] **Step 1: Build**

```bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:/c/gbdk/bin:$PATH" && make
```

Expected: clean build, ROM at `build/gameboygame.gb`.

- [ ] **Step 2: Run in mGBA + cycle scenes**

Open `build/gameboygame.gb` in mGBA. Press START 5 times. Verify the screen text cycles:
- Boot → "SCENE TITLE / START -> PLAY"
- START → "SCENE PLAY / START -> WIN"
- START → "SCENE WIN / START -> LOSE"
- START → "SCENE LOSE / START -> ALL DONE"
- START → "SCENE ALL DONE / START -> TITLE"
- START → back to TITLE

If a scene doesn't switch, the most likely cause is the scene table not being populated correctly — check Task 1.3 Step 1's imperative assignment block.

### Task 1.6: Commit Phase 1

- [ ] **Step 1: Stage and commit**

```bash
git add Makefile src/main.c src/game/scene.h src/game/scene_title.c src/game/scene_play.c src/game/scene_win.c src/game/scene_lose.c src/game/scene_all_done.c
git commit -m "B1: scene dispatch infrastructure — scene table + VBlank ISR + 5 scene stubs"
```

- [ ] **Step 2: Update Phase 1's Execution Status banner above + the top-of-plan table**

---

## Phase 2 — TDD compute_play_layout() pure function

**Execution Status:** ⬜ NOT STARTED

**Goal**: Build the dynamic compacting layout calculator — a pure C function that takes `groups_solved` (bits 0..3) and returns where each of the 16 unsolved cells and 0..3 solved bars should appear on screen. Pure function = no GBDK includes = host-testable with vanilla gcc. End state: `make test` includes a new test binary that verifies layout for all 4 possible `groups_solved` counts (0, 1, 2, 3 — case 4 triggers PLAY→WIN before layout is recomputed).

**BEFORE starting work on this phase:**

1. Invoke `/superpowers:test-driven-development`

This phase is pure-function TDD identical in shape to Plan A Phase 7. Follow red-green-refactor for every function.

### Task 2.1: Define layout.h API

**Files:**
- Create: `src/game/layout.h`

- [ ] **Step 1: Write the header**

```c
// src/game/layout.h
#ifndef GAME_LAYOUT_H
#define GAME_LAYOUT_H

#include <stdint.h>

#define LAYOUT_CELL_W    9   // tile columns per unsolved cell (incl. border)
#define LAYOUT_SCREEN_W  20  // total tile columns on screen
#define LAYOUT_SCREEN_H  18  // total tile rows on screen

// Maximum solved bars stacked at the top. 4 would mean PLAY ends; UI shows
// up to 3 simultaneously.
#define LAYOUT_MAX_BARS  4

typedef struct {
    // Solved bars at the top. bars_count == popcount(groups_solved).
    // Each bar occupies one row, indexed by tier (0=yellow .. 3=purple).
    uint8_t bars_count;
    uint8_t bar_tier[LAYOUT_MAX_BARS];     // tier index of each bar (0..3), in top-to-bottom order
    uint8_t bar_y[LAYOUT_MAX_BARS];        // tilemap row of each bar

    // Where the 16 unsolved-cell slots live. Cells solved-out of the puzzle
    // are NOT included in this list — only the (16 - 4*bars_count) remaining
    // unsolved cells. cells_count is set accordingly.
    uint8_t cells_count;
    uint8_t cell_x[16];                     // tilemap col of each remaining cell's top-left
    uint8_t cell_y[16];                     // tilemap row of each remaining cell's top-left
    uint8_t cell_h;                         // tilemap rows per remaining cell (varies with bars_count)
} LayoutInfo;

// Populate *out for the given groups_solved bitmask (bits 0..3).
// Returns false if groups_solved has more than 3 bits set (caller should
// transition to WIN before layout for 4 solved is meaningful).
#include <stdbool.h>
bool compute_play_layout(uint8_t groups_solved, LayoutInfo *out);

#endif
```

**About cell_h:** the design spec §8 says cell height grows as more groups solve:
- 0 solved → 16 cells in 8 rows × 2 cols, each cell ~2 rows tall
- 1 solved → 12 cells in 6 rows × 2 cols, each cell ~2 rows tall  
- 2 solved → 8 cells in 4 rows × 2 cols, each cell ~3 rows tall
- 3 solved → 4 cells in 2 rows × 2 cols, each cell ~5 rows tall

Tile rows (not pixels) — DMG screen is 18 rows total, header reserves 1 row, each solved bar takes 1 row, leaving (17 - bars_count) rows for unsolved cells.

### Task 2.2: Stub layout.c (TDD red phase setup)

**Files:**
- Create: `src/game/layout.c`

- [ ] **Step 1: Write the stub that returns false/zero**

```c
// src/game/layout.c
#include "layout.h"
#include <string.h>

bool compute_play_layout(uint8_t groups_solved, LayoutInfo *out) {
    (void)groups_solved; (void)out;
    memset(out, 0, sizeof(*out));
    return false;  // intentionally wrong — TDD red phase
}
```

### Task 2.3: Update test/Makefile to add the layout test

**Files:**
- Modify: `test/Makefile`

- [ ] **Step 1: Add the layout test binary**

Insert after the existing `test_puzzle_logic` rule:

```makefile
test: test_puzzle_logic test_layout
	./test_puzzle_logic
	./test_layout

test_layout: test_layout.c ../src/game/layout.c
	@mkdir -p /tmp
	$(CC) $(CFLAGS) -o $@ $^
```

Also update the `clean:` recipe to remove `test_layout`:

```makefile
clean:
	rm -f test_puzzle_logic test_puzzle_logic.exe test_layout test_layout.exe
```

### Task 2.4: Write the first failing test (TDD red)

**Files:**
- Create: `test/test_layout.c`

- [ ] **Step 1: Write the test framework + first test**

```c
// test/test_layout.c
#include "game/layout.h"
#include <stdio.h>

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { tests_failed++; fprintf(stderr, "FAIL: %s (line %d): %s\n", __func__, __LINE__, msg); } \
} while (0)

#define ASSERT_EQ(actual, expected, msg) do { \
    tests_run++; \
    if ((actual) != (expected)) { tests_failed++; fprintf(stderr, "FAIL: %s (line %d): %s — expected %d, got %d\n", __func__, __LINE__, msg, (int)(expected), (int)(actual)); } \
} while (0)

static void test_layout_zero_solved(void) {
    LayoutInfo lay;
    bool ok = compute_play_layout(0x00, &lay);
    ASSERT_TRUE(ok, "0 solved should be a valid layout");
    ASSERT_EQ(lay.bars_count, 0, "0 solved → 0 bars");
    ASSERT_EQ(lay.cells_count, 16, "0 solved → 16 cells");
}

int main(void) {
    test_layout_zero_solved();
    printf("Tests run: %d, Failed: %d\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
```

- [ ] **Step 2: Run test, verify it fails for the right reason**

```bash
cd test && export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH" && make
```

Expected: 3 failed assertions (`ok==false`, `bars_count==0` happens to pass since stub memsets to 0, `cells_count==16` fails). The interesting failure is "0 solved should be a valid layout" — confirms the stub returns false where the real implementation should return true.

### Task 2.5: TDD green — implement 0-solved case

**Files:**
- Modify: `src/game/layout.c`

- [ ] **Step 1: Implement enough to pass the first test**

Replace the stub body:

```c
bool compute_play_layout(uint8_t groups_solved, LayoutInfo *out) {
    memset(out, 0, sizeof(*out));

    // Count solved groups (popcount of low 4 bits)
    uint8_t bars = 0;
    for (uint8_t i = 0; i < 4; i++) {
        if (groups_solved & (1u << i)) bars++;
    }
    if (bars > 3) return false;  // 4 solved → caller should transition to WIN

    out->bars_count = bars;
    out->cells_count = (uint8_t)(16 - bars * 4);
    return true;
}
```

- [ ] **Step 2: Run test, verify GREEN**

```bash
make
```

Expected: `Tests run: 3, Failed: 0` (or whatever the count is).

### Task 2.6: TDD bar placement (which tiers, which rows)

**Files:**
- Modify: `test/test_layout.c`, then `src/game/layout.c`

- [ ] **Step 1: Add failing tests for bar placement**

```c
static void test_layout_one_solved_yellow(void) {
    // Only bit 0 set → yellow solved → 1 bar at top, tier 0
    LayoutInfo lay;
    compute_play_layout(0x01, &lay);
    ASSERT_EQ(lay.bars_count, 1, "1 group solved → 1 bar");
    ASSERT_EQ(lay.bar_tier[0], 0, "yellow → tier 0");
    ASSERT_EQ(lay.bar_y[0], 0, "first bar at row 0");
}

static void test_layout_two_solved_yellow_blue(void) {
    // Bits 0 and 2 set → yellow + blue solved
    LayoutInfo lay;
    compute_play_layout(0x05, &lay);
    ASSERT_EQ(lay.bars_count, 2, "2 groups solved → 2 bars");
    ASSERT_EQ(lay.bar_tier[0], 0, "first bar = yellow (lower tier index)");
    ASSERT_EQ(lay.bar_tier[1], 2, "second bar = blue");
    ASSERT_EQ(lay.bar_y[0], 0, "first bar at row 0");
    ASSERT_EQ(lay.bar_y[1], 1, "second bar at row 1");
}

static void test_layout_three_solved_all_but_purple(void) {
    LayoutInfo lay;
    compute_play_layout(0x07, &lay);
    ASSERT_EQ(lay.bars_count, 3, "3 groups solved → 3 bars");
    ASSERT_EQ(lay.bar_tier[0], 0, "tier 0 first");
    ASSERT_EQ(lay.bar_tier[1], 1, "tier 1 second");
    ASSERT_EQ(lay.bar_tier[2], 2, "tier 2 third");
}

static void test_layout_four_solved_invalid(void) {
    LayoutInfo lay;
    bool ok = compute_play_layout(0x0F, &lay);
    ASSERT_TRUE(!ok, "4 groups solved → not a valid PLAY layout");
}
```

Add calls to main(). Run; expect bar_tier and bar_y tests to fail (stub doesn't set them).

- [ ] **Step 2: Implement bar placement**

Replace the implementation:

```c
bool compute_play_layout(uint8_t groups_solved, LayoutInfo *out) {
    memset(out, 0, sizeof(*out));

    uint8_t bars = 0;
    for (uint8_t tier = 0; tier < 4; tier++) {
        if (groups_solved & (1u << tier)) {
            if (bars >= LAYOUT_MAX_BARS) break;  // shouldn't hit, but defensive
            out->bar_tier[bars] = tier;
            out->bar_y[bars] = bars;  // stacked at top, one tile row per bar
            bars++;
        }
    }
    if (bars > 3) return false;
    out->bars_count = bars;
    out->cells_count = (uint8_t)(16 - bars * 4);
    return true;
}
```

- [ ] **Step 3: Run, verify all green**

```bash
make
```

### Task 2.7: TDD cell positioning

**Files:**
- Modify: `test/test_layout.c`, then `src/game/layout.c`

- [ ] **Step 1: Add tests for cell positions**

```c
static void test_layout_zero_solved_cell_positions(void) {
    LayoutInfo lay;
    compute_play_layout(0x00, &lay);
    // 16 cells in 8 rows × 2 cols. cell_h depends on available rows
    // (header reserves row 0, so 17 rows for cells = 17/8 = 2 rows per cell)
    ASSERT_EQ(lay.cell_h, 2, "0 bars → 2 tile rows per cell");
    // Cell 0 at top-left (col 1, row 1) — assuming 1-tile left/top margin
    ASSERT_EQ(lay.cell_x[0], 1, "cell 0 at col 1");
    ASSERT_EQ(lay.cell_y[0], 1, "cell 0 at row 1");
    // Cell 1 at top-right (col 11, row 1) — assuming 10-tile spacing
    ASSERT_EQ(lay.cell_x[1], 11, "cell 1 at col 11");
    ASSERT_EQ(lay.cell_y[1], 1, "cell 1 at row 1");
    // Cell 2 second row, left
    ASSERT_EQ(lay.cell_x[2], 1, "cell 2 at col 1");
    ASSERT_EQ(lay.cell_y[2], 3, "cell 2 at row 3");
}

static void test_layout_two_solved_cell_h(void) {
    LayoutInfo lay;
    compute_play_layout(0x05, &lay);
    // 8 cells remaining, 4 rows × 2 cols, 17-2=15 rows available → 15/4 ~ 3 rows
    ASSERT_EQ(lay.cell_h, 3, "2 bars → 3 tile rows per cell");
    ASSERT_EQ(lay.cells_count, 8, "8 cells remaining");
    // First cell below the 2 bars + 1 header row → row 3
    ASSERT_EQ(lay.cell_y[0], 3, "cell 0 at row 3 (below header + 2 bars)");
}
```

- [ ] **Step 2: Implement cell positioning**

Add to the bottom of `compute_play_layout`, after the bar-placement loop:

```c
    // Cell grid: 2 columns, (cells_count / 2) rows. Cells fill the area
    // below the bars (which occupy rows 0..bars-1).
    // We reserve row `bars` as a 1-tile gap/header buffer between bars
    // and the cell grid.
    uint8_t cell_rows = (uint8_t)(out->cells_count / 2);
    uint8_t available_rows = (uint8_t)(LAYOUT_SCREEN_H - (bars + 1));
    out->cell_h = (uint8_t)(available_rows / cell_rows);  // tile rows per cell

    uint8_t cell_top = (uint8_t)(bars + 1);  // first row after bars + 1-tile gap
    for (uint8_t i = 0; i < out->cells_count; i++) {
        uint8_t r = (uint8_t)(i / 2);
        uint8_t c = (uint8_t)(i % 2);
        out->cell_x[i] = (uint8_t)(1 + c * 10);
        out->cell_y[i] = (uint8_t)(cell_top + r * out->cell_h);
    }
    return true;
}
```

- [ ] **Step 3: Run, verify green**

```bash
make
```

Expected: all tests pass.

### Task 2.8: Wire test-logic to include the new layout test

**Files:**
- Modify: top-level `Makefile`

- [ ] **Step 1: Verify `make test` includes the layout test**

The top-level Makefile's `test-logic` target already delegates to `test/Makefile`, and we updated `test/Makefile` in Task 2.3 to include `test_layout`. From project root:

```bash
make test
```

Expected: both `test_puzzle_logic` and `test_layout` run, both pass. Python codegen tests still pass.

### Task 2.9: Commit Phase 2

- [ ] **Step 1: Stage and commit**

```bash
git add test/Makefile test/test_layout.c src/game/layout.h src/game/layout.c
git commit -m "B2: implement compute_play_layout with TDD — pure function, host-testable"
```

- [ ] **Step 2: Update Phase 2 banner above**

---

## Phase 3 — UI tile assets + title art

**Execution Status:** ⬜ NOT STARTED

**Goal**: Generate `assets/ui_tiles.png` (cursor sprite tiles, cell border tiles, 4 tier pattern tiles, solid label tile) and `assets/title.png` (title screen art). Wire png2asset into the Makefile for both. Load all tiles into VRAM at boot. End state: a smoke test in main.c temporarily renders one of each new tile to confirm they're loaded correctly.

### Task 3.1: Extend make_font.py into make_ui_tiles.py

**Files:**
- Create: `tools/make_ui_tiles.py`

The same approach as Phase 2 of Plan A — generate the PNG programmatically with a pure-stdlib PNG writer. Reuses the PNG-writer functions from `make_font.py`, adapted for a smaller image.

- [ ] **Step 1: Write the UI tile generator**

```python
#!/usr/bin/env python3
"""make_ui_tiles.py — Generate assets/ui_tiles.png.

Output: a 128x16 indexed-color PNG with the DMG 4-color palette,
arranged as 16 columns × 2 rows of 8x8 tiles (32 tiles total).

Tile layout (left to right, top row first):
    Row 0:
      Tile  0: cursor (solid border, hollow center)
      Tile  1: cell border top-left corner
      Tile  2: cell border top-right corner
      Tile  3: cell border bottom-left corner
      Tile  4: cell border bottom-right corner
      Tile  5: cell border horizontal edge (top + bottom)
      Tile  6: cell border vertical edge (left + right)
      Tile  7: solid fill (cell interior background)
      Tile  8: selected-cell highlight fill
      Tile  9: tier pattern 0 (yellow — sparse dots)
      Tile 10: tier pattern 1 (green — horizontal stripes)
      Tile 11: tier pattern 2 (blue — vertical stripes)
      Tile 12: tier pattern 3 (purple — checker)
      Tile 13: solved-bar label background (solid darkest)
      Tile 14: reserved
      Tile 15: reserved
    Row 1: all reserved (future expansion)

Each tile is encoded as 8 rows of pixel data, where each pixel is a
palette index 0..3 (0=lightest, 3=darkest matching DMG palette).
"""

import struct
import sys
import zlib
from pathlib import Path

PALETTE = [
    (0x9b, 0xbc, 0x0f),
    (0x8b, 0xac, 0x0f),
    (0x30, 0x62, 0x30),
    (0x0f, 0x38, 0x0f),
]

# Each tile is an 8-row × 8-col list of palette indices.
# Indexing: tile[row][col]
def _tile_cursor():
    # Solid 1-pixel border, hollow center
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            if r == 0 or r == 7 or c == 0 or c == 7:
                row.append(3)  # darkest border
            else:
                row.append(0)  # transparent-equivalent (background)
        rows.append(row)
    return rows

def _tile_corner_tl():
    # Top-left corner: dark border on top + left edges only
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            if r == 0 or c == 0:
                row.append(3)
            else:
                row.append(0)
        rows.append(row)
    return rows

def _tile_corner_tr():
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            if r == 0 or c == 7:
                row.append(3)
            else:
                row.append(0)
        rows.append(row)
    return rows

def _tile_corner_bl():
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            if r == 7 or c == 0:
                row.append(3)
            else:
                row.append(0)
        rows.append(row)
    return rows

def _tile_corner_br():
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            if r == 7 or c == 7:
                row.append(3)
            else:
                row.append(0)
        rows.append(row)
    return rows

def _tile_edge_horiz():
    # Border on top edge only (used as the bottom edge of cells above + top edge of cells below)
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            row.append(3 if r == 0 else 0)
        rows.append(row)
    return rows

def _tile_edge_vert():
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            row.append(3 if c == 0 else 0)
        rows.append(row)
    return rows

def _tile_solid_light():
    return [[0]*8 for _ in range(8)]

def _tile_selected_highlight():
    # Diagonal-cross pattern in shade 2 — visibly different from default fill
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            row.append(2 if ((r + c) % 4 == 0) else 0)
        rows.append(row)
    return rows

def _tile_pattern_yellow():
    # Sparse dots: dark pixel every 4 in both axes
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            row.append(3 if (r % 4 == 1 and c % 4 == 1) else 0)
        rows.append(row)
    return rows

def _tile_pattern_green():
    # Horizontal stripes: dark on even rows
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            row.append(3 if r % 2 == 0 else 0)
        rows.append(row)
    return rows

def _tile_pattern_blue():
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            row.append(3 if c % 2 == 0 else 0)
        rows.append(row)
    return rows

def _tile_pattern_purple():
    # 2x2 checker
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            row.append(3 if ((r // 2) + (c // 2)) % 2 == 0 else 0)
        rows.append(row)
    return rows

def _tile_solid_dark():
    return [[3]*8 for _ in range(8)]

def _tile_reserved():
    return _tile_solid_light()

TILES = [
    _tile_cursor(),
    _tile_corner_tl(),
    _tile_corner_tr(),
    _tile_corner_bl(),
    _tile_corner_br(),
    _tile_edge_horiz(),
    _tile_edge_vert(),
    _tile_solid_light(),
    _tile_selected_highlight(),
    _tile_pattern_yellow(),
    _tile_pattern_green(),
    _tile_pattern_blue(),
    _tile_pattern_purple(),
    _tile_solid_dark(),
    _tile_reserved(),
    _tile_reserved(),
]
TILES += [_tile_reserved() for _ in range(16)]  # row 1 reserved

WIDTH = 128
HEIGHT = 16

assert len(TILES) == 32, f"TILES must have 32 entries, got {len(TILES)}"

# (Reuse render_pixels / pack_2bpp_scanline / png_chunk / write_png helpers from make_font.py.
#  See make_font.py for the canonical implementation.)
sys.path.insert(0, str(Path(__file__).parent))
from make_font import pack_2bpp_scanline, png_chunk

def render_pixels():
    px = [[0] * WIDTH for _ in range(HEIGHT)]
    for idx, tile in enumerate(TILES):
        tx = idx % 16
        ty = idx // 16
        for r in range(8):
            for c in range(8):
                px[ty * 8 + r][tx * 8 + c] = tile[r][c]
    return px

def write_png(path, pixels):
    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 2, 3, 0, 0, 0)
    plte = b"".join(struct.pack("BBB", *c) for c in PALETTE)
    raw = bytearray()
    for row in pixels:
        raw.append(0)
        raw.extend(pack_2bpp_scanline(row))
    idat = zlib.compress(bytes(raw), 9)
    body = sig + png_chunk(b"IHDR", ihdr) + png_chunk(b"PLTE", plte) + png_chunk(b"IDAT", idat) + png_chunk(b"IEND", b"")
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_bytes(body)

def main():
    output = sys.argv[1] if len(sys.argv) > 1 else "assets/ui_tiles.png"
    write_png(output, render_pixels())
    print(f"Wrote {output} ({WIDTH}x{HEIGHT}, 2bpp indexed, {len(TILES)} tiles)")

if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Run the generator + verify the PNG**

```bash
python tools/make_ui_tiles.py
file assets/ui_tiles.png
```

Expected: `assets/ui_tiles.png: PNG image data, 128 x 16, 2-bit colormap, non-interlaced`.

### Task 3.2: Generate title.png

**Files:**
- Create: `tools/make_title.py`

The title screen is 160×144 pixels = 20×18 tiles = 360 tiles. Out of scope to hand-pixel a full title image, so generate a simple programmatic title — large block letters spelling "CONNECTIONS" plus decorative borders.

- [ ] **Step 1: Write a minimal title generator**

```python
#!/usr/bin/env python3
"""make_title.py — Generate assets/title.png (simple programmatic title screen).

Outputs a 160x144 indexed PNG with the title text rendered as oversized
block letters. Plan C will polish this; Plan B just needs something
visually distinguishable from the gameplay scenes.
"""

import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from make_font import pack_2bpp_scanline, png_chunk, FONT, PALETTE

WIDTH = 160
HEIGHT = 144

def render_pixels():
    px = [[0] * WIDTH for _ in range(HEIGHT)]
    # Border: dark frame 2 pixels thick
    for r in range(HEIGHT):
        for c in range(WIDTH):
            if r < 2 or r >= HEIGHT - 2 or c < 2 or c >= WIDTH - 2:
                px[r][c] = 3
    # Title text "CONNECTIONS" rendered as 2x-scale font glyphs
    # 11 chars × 16px = 176px, won't fit at 2x — use 1x and center
    title = "GBCX"
    char_w = 8
    title_x = (WIDTH - len(title) * char_w) // 2
    title_y = 30
    for i, ch in enumerate(title):
        if ord(ch) < 0x20 or ord(ch) > 0x5F:
            continue
        glyph = FONT[ord(ch) - 0x20]
        for r in range(8):
            row_byte = glyph[r]
            for c in range(8):
                if row_byte & (1 << c):
                    # 2x scale
                    for dr in range(2):
                        for dc in range(2):
                            y = title_y + r * 2 + dr
                            x = title_x + i * char_w * 2 + c * 2 + dc
                            if 0 <= y < HEIGHT and 0 <= x < WIDTH:
                                px[y][x] = 3
    # Subtitle: "CONNECTIONS" (smaller, below)
    subtitle = "CONNECTIONS"
    sub_x = (WIDTH - len(subtitle) * char_w) // 2
    sub_y = 80
    for i, ch in enumerate(subtitle):
        if ord(ch) < 0x20 or ord(ch) > 0x5F:
            continue
        glyph = FONT[ord(ch) - 0x20]
        for r in range(8):
            row_byte = glyph[r]
            for c in range(8):
                if row_byte & (1 << c):
                    y = sub_y + r
                    x = sub_x + i * char_w + c
                    if 0 <= y < HEIGHT and 0 <= x < WIDTH:
                        px[y][x] = 3
    return px

def write_png(path, pixels):
    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 2, 3, 0, 0, 0)
    plte = b"".join(struct.pack("BBB", *c) for c in PALETTE)
    raw = bytearray()
    for row in pixels:
        raw.append(0)
        raw.extend(pack_2bpp_scanline(row))
    idat = zlib.compress(bytes(raw), 9)
    body = sig + png_chunk(b"IHDR", ihdr) + png_chunk(b"PLTE", plte) + png_chunk(b"IDAT", idat) + png_chunk(b"IEND", b"")
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_bytes(body)

def main():
    output = sys.argv[1] if len(sys.argv) > 1 else "assets/title.png"
    write_png(output, render_pixels())
    print(f"Wrote {output} ({WIDTH}x{HEIGHT})")

if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Generate title.png**

```bash
python tools/make_title.py
file assets/title.png
```

Expected: `assets/title.png: PNG image data, 160 x 144, 2-bit colormap, non-interlaced`.

### Task 3.3: Wire png2asset into Makefile for the new assets

**Files:**
- Modify: `Makefile`

- [ ] **Step 1: Extend the ASSETS_PNG list**

Replace:

```makefile
ASSETS_PNG := assets/font.png
```

with:

```makefile
ASSETS_PNG := assets/font.png assets/ui_tiles.png assets/title.png
```

The existing `src/assets_gen/%.c: assets/%.png` pattern rule will pick up the new files automatically. Plan A's flags (`-spr8x8 -bpp 2 -keep_palette_order -sprite_no_optimize -tiles_only -noflip`) apply to all three.

- [ ] **Step 2: Verify generation**

```bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:/c/gbdk/bin:$PATH" && make clean && make 2>&1 | tail -10
ls src/assets_gen/
```

Expected: `font.c font.h title.c title.h ui_tiles.c ui_tiles.h` all present.

### Task 3.4: Load UI tiles + title tiles into VRAM at boot

**Files:**
- Modify: `src/engine/render.h`, `src/engine/render.c`

The current `render_init` loads only font tiles starting at tile index 0. Plan B needs more tiles available; layout them in VRAM as follows:

| VRAM tile range | Source |
|---|---|
| 0..63 | font (ASCII 0x20–0x5F, 64 tiles) |
| 64..95 | ui_tiles (32 tiles) |
| 96+   | title (Plan B uses only when TITLE scene is active; tiles loaded on demand by `render_load_title_tiles()`) |

- [ ] **Step 1: Update render.h with new API**

Add to `src/engine/render.h`:

```c
// Tile indices for UI tile sheet (offset from VRAM start)
#define UI_TILE_BASE 64

#define UI_TILE_CURSOR        (UI_TILE_BASE + 0)
#define UI_TILE_CORNER_TL     (UI_TILE_BASE + 1)
#define UI_TILE_CORNER_TR     (UI_TILE_BASE + 2)
#define UI_TILE_CORNER_BL     (UI_TILE_BASE + 3)
#define UI_TILE_CORNER_BR     (UI_TILE_BASE + 4)
#define UI_TILE_EDGE_HORIZ    (UI_TILE_BASE + 5)
#define UI_TILE_EDGE_VERT     (UI_TILE_BASE + 6)
#define UI_TILE_FILL_LIGHT    (UI_TILE_BASE + 7)
#define UI_TILE_FILL_SEL      (UI_TILE_BASE + 8)
#define UI_TILE_PATTERN_BASE  (UI_TILE_BASE + 9)   // +0..+3 = yellow, green, blue, purple
#define UI_TILE_SOLID_DARK    (UI_TILE_BASE + 13)

// Load title screen tiles (call before SCENE_TITLE; takes ~360 tiles).
// CAUTION: this OVERWRITES the UI tiles in VRAM. Call render_init()
// to restore them when transitioning out of TITLE.
void render_load_title_tiles(void);

// Restore the font + UI tile layout (called after exiting TITLE).
void render_restore_default_tiles(void);

// Set a single tile at (x, y) in the tilemap buffer.
void render_set_tile(uint8_t x, uint8_t y, uint8_t tile);

// Draw a tilemap region by tile indices (not text). For UI chrome.
void render_draw_tiles(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *tiles);
```

- [ ] **Step 2: Update render.c**

```c
#include "../assets_gen/font.h"
#include "../assets_gen/ui_tiles.h"
#include "../assets_gen/title.h"

void render_init(void) {
    set_bkg_data(0, font_TILE_COUNT, font_tiles);
    set_bkg_data(UI_TILE_BASE, ui_tiles_TILE_COUNT, ui_tiles_tiles);
    render_clear();
}

void render_load_title_tiles(void) {
    set_bkg_data(0, title_TILE_COUNT, title_tiles);
    render_clear();
}

void render_restore_default_tiles(void) {
    set_bkg_data(0, font_TILE_COUNT, font_tiles);
    set_bkg_data(UI_TILE_BASE, ui_tiles_TILE_COUNT, ui_tiles_tiles);
    render_clear();
}

void render_set_tile(uint8_t x, uint8_t y, uint8_t tile) {
    if (x >= SCREEN_TILES_W || y >= SCREEN_TILES_H) return;
    extern uint8_t tilemap_buf[SCREEN_TILES_W * SCREEN_TILES_H];
    extern uint8_t dirty;
    tilemap_buf[y * SCREEN_TILES_W + x] = tile;
    dirty = 1;
}

void render_draw_tiles(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *tiles) {
    for (uint8_t dy = 0; dy < h; dy++) {
        for (uint8_t dx = 0; dx < w; dx++) {
            render_set_tile(x + dx, y + dy, tiles[dy * w + dx]);
        }
    }
}
```

The `extern uint8_t tilemap_buf[]; extern uint8_t dirty;` declarations break the existing `static`-scoping. To preserve encapsulation, instead move `tilemap_buf` and `dirty` to the top of render.c without `static`, and remove the `extern` declarations inside the function bodies. This is a minor refactor — verify with the build step below that nothing else broke.

- [ ] **Step 3: Build, verify it compiles**

```bash
make 2>&1 | tail -10
```

Expected: clean build.

### Task 3.5: Temporary smoke test for UI tiles

**Files:**
- Modify: `src/main.c` (temporary — Phase 4's TITLE scene will replace this)

- [ ] **Step 1: Add a temporary tile-display block to one of the scene stubs**

Modify `src/game/scene_title.c`'s `title_init()` to draw the new UI tiles in a visible row:

```c
static void title_init(void) {
    render_clear();
    render_text(2, 1, "PHASE 3 TILE TEST");
    render_text(2, 3, "UI TILES (row 5):");

    // Display tiles 64..76 horizontally on row 5
    for (uint8_t i = 0; i < 13; i++) {
        render_set_tile(2 + i, 5, UI_TILE_BASE + i);
    }

    render_text(2, 7, "START -> PLAY");
}
```

- [ ] **Step 2: Build + verify in mGBA**

```bash
make
```

Open in mGBA. Expected: row 5 shows 13 small distinct tiles — cursor (hollow square), 4 corner shapes, 2 edges, 2 fill patterns (light, selected), 4 tier patterns. If any tile looks corrupted, the most likely cause is the VRAM offset — verify `UI_TILE_BASE == 64`.

### Task 3.6: Commit Phase 3

- [ ] **Step 1: Stage and commit**

```bash
git add Makefile assets/ui_tiles.png assets/title.png tools/make_ui_tiles.py tools/make_title.py src/engine/render.h src/engine/render.c src/game/scene_title.c
git commit -m "B3: UI tile assets + title art — cursor, borders, 4 tier patterns, title screen"
```

- [ ] **Step 2: Update Phase 3 banner above**

---

## Phase 4 — SCENE_TITLE

**Execution Status:** ⬜ NOT STARTED

**Goal**: Build the real TITLE scene — title art background + adaptive menu (CONTINUE/RESTART/NEW GAME based on save's `ip_tries_remaining`) + menu cursor with auto-repeat + NEW GAME confirm overlay + save writes at the 3 trigger points (CONTINUE = none, RESTART = clear ip_*, NEW GAME = full reset). End state: a polished title screen the user can navigate, with state-aware menu items and confirmed destructive operations.

### Task 4.1: Add scene_title state struct

**Files:**
- Modify: `src/game/scene_title.c`

- [ ] **Step 1: Replace the stub with TITLE-scene state**

```c
#include "scene.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include <stdio.h>

typedef enum {
    TITLE_MENU_CONTINUE = 0,
    TITLE_MENU_RESTART,
    TITLE_MENU_NEW_GAME,
    TITLE_MENU_COUNT
} TitleMenuItem;

typedef struct {
    GameSave save;
    bool has_in_progress;   // computed from save.ip_tries_remaining > 0
    uint8_t menu_cursor;
    uint8_t menu_item_count;        // 2 if no in-progress (CONTINUE/NEW), 3 if in-progress (CONTINUE/RESTART/NEW)
    bool show_confirm;              // NEW GAME confirm overlay
} TitleState;

static TitleState ts;
```

### Task 4.2: Implement title_init

**Files:**
- Modify: `src/game/scene_title.c`

- [ ] **Step 1: Replace title_init**

```c
static void title_init(void) {
    save_load(&ts.save);
    ts.has_in_progress = (ts.save.ip_tries_remaining > 0);
    ts.menu_cursor = 0;
    ts.menu_item_count = ts.has_in_progress ? 3 : 2;
    ts.show_confirm = false;

    render_load_title_tiles();  // overwrites font + UI tiles; restored on teardown

    // Title art is loaded as tiles 0..N from title.png. Map the full
    // 20x18 screen to those tiles.
    // (title.png is 160x144 = 20x18 tiles = 360 tiles; if dedup happened
    // it'll be fewer, but the tilemap is still 20x18 cells. We'll draw
    // it as a sequential mapping: tilemap_buf[y*20+x] = y*20+x.)
    for (uint8_t y = 0; y < 18; y++) {
        for (uint8_t x = 0; x < 20; x++) {
            render_set_tile(x, y, (uint8_t)(y * 20 + x));
        }
    }
}
```

**About the title tile mapping:** if `make_title.py` produces a 20×18-cell image and png2asset emits it with `-tiles_only` in row-major order (without dedup, which is disabled by `-sprite_no_optimize`), then tile `i` corresponds to pixel cell `(i % 20, i / 20)`. The mapping above just renders the source image 1:1.

Wait — `title.png` is 160×144 pixels, but we use png2asset's `-sprite_no_optimize` flag which disables dedup. The PNG has 20×18 = 360 8×8 cells. SDCC's compiled output is `title_tiles[360 * 16] = ...` (5760 bytes). At 64 tiles font + 32 tiles UI + 360 tiles title = 456 background tiles, but VRAM only holds 256 background tile patterns. So `render_load_title_tiles()` ONLY loads title tiles, overwriting font + UI tiles. That's fine — the title scene doesn't need to render text via the font subsystem because the title art IS the screen content. On exiting TITLE, `render_restore_default_tiles()` reloads font + UI.

The menu text overlay is a problem with this approach — the title screen has menu items as live text, but we just overwrote the font tiles. Resolution: render the menu text into the title.png itself as part of the title screen art, OR load font tiles to a different VRAM offset. The simplest fix: include menu glyphs in title.png at known cell positions, and update title_init to map those cells based on menu state.

For simplicity in Plan B, we'll take a different approach: only load title.png's BACKGROUND art (top half of the screen), then load font tiles to a different VRAM range so menu text can be rendered on top.

Actually, the cleanest minimum-complexity approach: don't load title.png at all in Plan B. Use the font subsystem to draw "GBCX / CONNECTIONS" as text, plus menu items, all using font tiles. Title art polish is deferred to Plan C.

**REVISED Task 4.2 Step 1:**

```c
static void title_init(void) {
    save_load(&ts.save);
    ts.has_in_progress = (ts.save.ip_tries_remaining > 0);
    ts.menu_cursor = 0;
    ts.menu_item_count = ts.has_in_progress ? 3 : 2;
    ts.show_confirm = false;
    // Default tile layout (font + UI) is already loaded by render_init()
    // from main.c. No need to load title.png in Plan B — title is text-only.
}
```

This means Task 3.2's title.png is unused in Plan B. Keep it generated for Plan C; don't load it at runtime. Update the Phase 3 task description if executed before this realization is hit.

### Task 4.3: Implement title_render

**Files:**
- Modify: `src/game/scene_title.c`

- [ ] **Step 1: Implement render with the menu state**

```c
static void title_render(void) {
    render_clear();
    render_text(7, 2, "GBCX");
    render_text(4, 4, "CONNECTIONS");

    uint8_t row = 9;
    if (ts.show_confirm) {
        render_text(2, 8,  "NEW GAME?");
        render_text(2, 10, "  ALL DATA LOST");
        render_text(2, 13, "A  CONFIRM");
        render_text(2, 14, "B  CANCEL");
        return;
    }

    if (ts.has_in_progress) {
        char buf[21];
        sprintf(buf, "%cCONTINUE P%d", ts.menu_cursor == 0 ? '>' : ' ', ts.save.current_puzzle_index + 1);
        render_text(2, row + 0, buf);
        sprintf(buf, "%cRESTART P%d",  ts.menu_cursor == 1 ? '>' : ' ', ts.save.current_puzzle_index + 1);
        render_text(2, row + 1, buf);
        sprintf(buf, "%cNEW GAME",     ts.menu_cursor == 2 ? '>' : ' ');
        render_text(2, row + 2, buf);
    } else {
        char buf[21];
        sprintf(buf, "%cCONTINUE",     ts.menu_cursor == 0 ? '>' : ' ');
        render_text(2, row + 0, buf);
        sprintf(buf, "%cNEW GAME",     ts.menu_cursor == 1 ? '>' : ' ');
        render_text(2, row + 1, buf);
    }

    // Stats footer
    char stats[21];
    sprintf(stats, "SOLVED:%d  BEST:%d", ts.save.puzzles_solved_total, ts.save.best_streak);
    render_text(2, 16, stats);
}
```

### Task 4.4: Implement title_update

**Files:**
- Modify: `src/game/scene_title.c`

- [ ] **Step 1: Implement update logic**

```c
static void title_update(Scene *next_scene) {
    if (ts.show_confirm) {
        if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
            // Confirm NEW GAME — wipe save, transition to PLAY
            save_reset(&ts.save);
            save_store(&ts.save);
            sfx_select();
            *next_scene = SCENE_PLAY;
        } else if (input_pressed(BTN_B)) {
            ts.show_confirm = false;
            sfx_deselect();
        }
        return;
    }

    // Menu navigation
    if (input_repeat(BTN_UP)) {
        ts.menu_cursor = (uint8_t)((ts.menu_cursor + ts.menu_item_count - 1) % ts.menu_item_count);
        sfx_move();
    }
    if (input_repeat(BTN_DOWN)) {
        ts.menu_cursor = (uint8_t)((ts.menu_cursor + 1) % ts.menu_item_count);
        sfx_move();
    }

    // Selection
    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
        if (ts.has_in_progress) {
            if (ts.menu_cursor == 0) {
                // CONTINUE — no save change
                sfx_select();
                *next_scene = SCENE_PLAY;
            } else if (ts.menu_cursor == 1) {
                // RESTART — clear in-progress fields, keep current_puzzle_index + lifetime stats
                ts.save.ip_tries_remaining = 0;
                ts.save.ip_groups_solved = 0;
                ts.save.ip_selected_mask = 0;
                ts.save.ip_elapsed_seconds = 0;
                save_store(&ts.save);
                sfx_select();
                *next_scene = SCENE_PLAY;
            } else {
                // NEW GAME — show confirm overlay first
                ts.show_confirm = true;
                sfx_select();
            }
        } else {
            if (ts.menu_cursor == 0) {
                sfx_select();
                *next_scene = SCENE_PLAY;
            } else {
                ts.show_confirm = true;
                sfx_select();
            }
        }
    }
}
```

### Task 4.5: Build + verify TITLE in mGBA

**Files:** none

- [ ] **Step 1: Build**

```bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:/c/gbdk/bin:$PATH" && make
```

- [ ] **Step 2: First-boot test (no in-progress)**

Open in mGBA. Expected:
- Title text "GBCX" / "CONNECTIONS" at top
- Two menu items: `>CONTINUE`, ` NEW GAME` (cursor on CONTINUE)
- Stats line at bottom: `SOLVED:0  BEST:0`
- UP/DOWN moves cursor between items
- A on NEW GAME shows confirm overlay
- B on confirm cancels back to menu
- A on confirm transitions to PLAY scene (stub from Phase 1 still says "SCENE PLAY")

- [ ] **Step 3: In-progress test**

To create an in-progress save, manually edit `build/gameboygame.sav` byte 13 (`ip_tries_remaining`) to a non-zero value in mGBA's memory viewer:
1. From PLAY stub, navigate back to TITLE via Phase 1's stub cycle (START → WIN → LOSE → ALL_DONE → TITLE)
2. Open mGBA Memory Viewer at `0xA000+13`
3. Change byte to `0x03` (3 tries remaining)
4. Recompute checksum: XOR all bytes 0..18 (or just hard-reset; on next boot, the corruption check will trigger save_reset)

Easier: just play one game stub → tries_remaining is auto-set to 4 → save persists. When you return to TITLE, you'll see the 3-item menu. This requires Phase 5+ for the real PLAY scene. Until then, manually corrupt the save byte 13 to test the in-progress path.

If the menu doesn't show 3 items, check `title_init`'s `has_in_progress` calculation.

### Task 4.6: Commit Phase 4

- [ ] **Step 1: Stage and commit**

```bash
git add src/game/scene_title.c
git commit -m "B4: SCENE_TITLE — adaptive menu + NEW GAME confirm + 3 save-write triggers"
```

- [ ] **Step 2: Update Phase 4 banner above**

---

## Phase 5 — SCENE_PLAY rendering + cursor + SELECT_FLASH

**Execution Status:** ⬜ NOT STARTED

**Goal**: Build the PLAY scene's static rendering — header (puzzle # / tries), 2×8 cell grid using Phase 3's UI tiles, cursor sprite navigation via D-pad with Plan A's auto-repeat, selection toggle on A, B clears selections. Includes the SELECT_FLASH animation (real implementation replacing the Phase 6 Plan A stub). NO submission logic yet (that's Phase 6). End state: a fully-rendered playable-looking PLAY scene where the user can move the cursor and select up to 4 cells with visual feedback, but pressing START does nothing yet.

### Task 5.1: Add scene_play state struct

**Files:**
- Modify: `src/game/scene_play.c`

- [ ] **Step 1: Replace the stub with PLAY-scene state**

```c
#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "puzzle_logic.h"
#include "layout.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include "../engine/anim.h"
#include <stdio.h>
#include <gb/gb.h>  // for set_sprite_tile + move_sprite

static PlayState ps;
static GameSave  pg_save;
static LayoutInfo pg_layout;

// VRAM tile index for the cursor sprite (loaded into OAM sprite tile bank).
// We use the cursor tile from ui_tiles (UI_TILE_CURSOR = 64 in the BACKGROUND
// tile bank). For sprites, GBDK uses set_sprite_data which writes to the
// sprite tile bank (0x8000-0x8FFF). For DMG, sprite tile bank IS the same
// physical VRAM as the background bank's first half. Tile index 64 referenced
// from a sprite IS the same data as background tile 64, so we can reuse
// UI_TILE_CURSOR directly.
#define CURSOR_SPRITE_INDEX 0
```

### Task 5.2: Implement play_init

**Files:**
- Modify: `src/game/scene_play.c`

- [ ] **Step 1: Implement init**

```c
static void play_init(void) {
    save_load(&pg_save);

    // Restore in-progress state if any, else fresh attempt
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
    ps.cursor_idx = 0;
    ps.show_quit_confirm = 0;

    compute_play_layout(ps.groups_solved, &pg_layout);

    // Configure cursor sprite
    set_sprite_tile(CURSOR_SPRITE_INDEX, UI_TILE_CURSOR);
}
```

### Task 5.3: Implement play_render (no submission feedback yet)

**Files:**
- Modify: `src/game/scene_play.c`

- [ ] **Step 1: Implement render with header + grid**

```c
static void render_cell(const Puzzle *puzzle, uint8_t cell_idx) {
    // cell_idx is 0..15 — the index into PUZZLES[].words[].
    // Find the slot this cell occupies in pg_layout (cells_count slots).
    // The N-th cell in pg_layout.cell_x/y corresponds to the N-th unsolved cell.
    // We need a mapping from cell_idx (0..15) → layout slot.
    // For Phase 5 (groups_solved == 0), this is identity.

    uint8_t slot = cell_idx;
    // TODO Phase 6: compute slot when groups_solved != 0 (skip solved-group words)

    uint8_t x = pg_layout.cell_x[slot];
    uint8_t y = pg_layout.cell_y[slot];
    uint8_t selected = (ps.selected_mask & (1u << cell_idx)) != 0;

    // Draw cell background: light fill or selected highlight
    uint8_t fill = selected ? UI_TILE_FILL_SEL : UI_TILE_FILL_LIGHT;
    for (uint8_t dy = 0; dy < pg_layout.cell_h; dy++) {
        for (uint8_t dx = 0; dx < 9; dx++) {  // 9-tile wide cell
            render_set_tile(x + dx, y + dy, fill);
        }
    }

    // Draw word centered horizontally
    const char *word = puzzle->words[cell_idx];
    uint8_t word_len = 0;
    while (word[word_len] && word_len < 9) word_len++;
    uint8_t text_x = x + (uint8_t)((9 - word_len) / 2);
    uint8_t text_y = y + (uint8_t)(pg_layout.cell_h / 2);
    render_text(text_x, text_y, word);
}

static void play_render(void) {
    render_clear();

    const Puzzle *puzzle = &PUZZLES[pg_save.current_puzzle_index];

    // Header: puzzle number + tries
    char hdr[21];
    sprintf(hdr, "P%d  TRIES:%d", pg_save.current_puzzle_index + 1, ps.tries_remaining);
    render_text(0, 0, hdr);

    // Draw all 16 cells (Phase 5: assume groups_solved == 0)
    for (uint8_t i = 0; i < 16; i++) {
        // Skip cells whose group is already solved
        uint8_t g = find_group_of_word(puzzle, i);
        if (ps.groups_solved & (1u << g)) continue;
        render_cell(puzzle, i);
    }

    if (ps.show_quit_confirm) {
        // Modal overlay — draw centered confirm
        for (uint8_t y = 6; y < 12; y++) {
            for (uint8_t x = 2; x < 18; x++) {
                render_set_tile(x, y, UI_TILE_FILL_LIGHT);
            }
        }
        render_text(4, 7, "QUIT TO TITLE?");
        render_text(4, 9, "A  YES");
        render_text(4, 10, "B  NO");
    }
}
```

### Task 5.4: Update cursor sprite position each frame

**Files:**
- Modify: `src/game/scene_play.c`

- [ ] **Step 1: Add a cursor position update**

The cursor sprite needs to follow `ps.cursor_idx`. Since the cursor highlights a single 8×8 tile (not the whole cell), we'll position it on the top-left tile of the current cell's content.

Add a helper:

```c
static void update_cursor_sprite(void) {
    // Find the slot for the current cursor cell
    // For Phase 5 (no solved groups), slot = cursor_idx
    uint8_t slot = ps.cursor_idx;
    uint8_t tile_x = pg_layout.cell_x[slot];
    uint8_t tile_y = pg_layout.cell_y[slot];

    // GBDK move_sprite uses pixel coordinates. Sprite origin is (8, 16) above
    // the top-left of the screen, so to position a sprite at tile (tx, ty)
    // its pixel position is (tx*8 + 8, ty*8 + 16).
    move_sprite(CURSOR_SPRITE_INDEX, (uint8_t)(tile_x * 8 + 8), (uint8_t)(tile_y * 8 + 16));
}
```

Call `update_cursor_sprite()` at the end of `play_init` and every time `ps.cursor_idx` changes in `play_update`.

### Task 5.5: Implement play_update (cursor navigation + selection, no submission)

**Files:**
- Modify: `src/game/scene_play.c`

- [ ] **Step 1: Implement update**

```c
static void play_update(Scene *next_scene) {
    if (anim_is_playing()) return;  // input locked during animations

    if (ps.show_quit_confirm) {
        if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
            // Save in-progress state so CONTINUE works on next boot
            pg_save.ip_tries_remaining = ps.tries_remaining;
            pg_save.ip_groups_solved   = ps.groups_solved;
            pg_save.ip_selected_mask   = ps.selected_mask;
            pg_save.ip_elapsed_seconds = ps.elapsed_seconds;
            save_store(&pg_save);
            *next_scene = SCENE_TITLE;
        } else if (input_pressed(BTN_B)) {
            ps.show_quit_confirm = 0;
            sfx_deselect();
        }
        return;
    }

    // Cursor nav (auto-repeat)
    if (input_repeat(BTN_UP)) {
        uint8_t row = ps.cursor_idx / 2, col = ps.cursor_idx % 2;
        row = (uint8_t)((row + 7) % 8);  // wraps
        ps.cursor_idx = (uint8_t)(row * 2 + col);
        // Skip over solved-group cells
        while (ps.groups_solved & (1u << find_group_of_word(&PUZZLES[pg_save.current_puzzle_index], ps.cursor_idx))) {
            row = (uint8_t)((row + 7) % 8);
            ps.cursor_idx = (uint8_t)(row * 2 + col);
        }
        update_cursor_sprite();
        sfx_move();
    }
    if (input_repeat(BTN_DOWN)) {
        uint8_t row = ps.cursor_idx / 2, col = ps.cursor_idx % 2;
        row = (uint8_t)((row + 1) % 8);
        ps.cursor_idx = (uint8_t)(row * 2 + col);
        while (ps.groups_solved & (1u << find_group_of_word(&PUZZLES[pg_save.current_puzzle_index], ps.cursor_idx))) {
            row = (uint8_t)((row + 1) % 8);
            ps.cursor_idx = (uint8_t)(row * 2 + col);
        }
        update_cursor_sprite();
        sfx_move();
    }
    if (input_pressed(BTN_LEFT)) {
        if (ps.cursor_idx % 2 == 1) {
            ps.cursor_idx--;
            update_cursor_sprite();
            sfx_move();
        }
    }
    if (input_pressed(BTN_RIGHT)) {
        if (ps.cursor_idx % 2 == 0) {
            ps.cursor_idx++;
            update_cursor_sprite();
            sfx_move();
        }
    }

    // Selection toggle
    if (input_pressed(BTN_A)) {
        if (toggle_selection(&ps, ps.cursor_idx)) {
            bool is_now_set = (ps.selected_mask & (1u << ps.cursor_idx)) != 0;
            // Trigger real SELECT_FLASH animation (data byte = cursor_idx)
            uint8_t data[8] = { ps.cursor_idx };
            anim_start(ANIM_SELECT_FLASH, data, 4);
            if (is_now_set) sfx_select(); else sfx_deselect();
        } else {
            sfx_reject();  // tried to select a 5th
        }
    }

    // B clears all selections
    if (input_pressed(BTN_B)) {
        if (ps.selected_mask != 0) {
            ps.selected_mask = 0;
            sfx_deselect();
        }
    }

    // SELECT triggers quit confirm
    if (input_pressed(BTN_SELECT)) {
        ps.show_quit_confirm = 1;
        sfx_select();
    }

    // Phase 5 stub: START → WIN cycle for testing scene transitions
    // (Phase 6 replaces this with real submission logic)
    if (input_pressed(BTN_START)) {
        // Don't transition yet — Phase 6 implements this
    }
}
```

### Task 5.6: Replace SELECT_FLASH stub in engine/anim.c

**Files:**
- Modify: `src/engine/anim.c`

The stub from Plan A Phase 6 just advances frame and ends. Real impl: flip the cell's background tile between fill-selected and fill-light for 4 frames, ending with whatever's appropriate based on actual selection state.

- [ ] **Step 1: Replace tick_select_flash with real implementation**

```c
static void tick_select_flash(void) {
    // active.data[0] = cursor_idx of the cell being toggled
    // Flash the cell's background tiles between FILL_SEL and FILL_LIGHT
    // every 2 frames for 4 frames total. After the anim ends, scene_play's
    // next render() will draw the correct state based on selected_mask.
    if (active.frame >= active.duration) {
        active.type = ANIM_NONE;
        return;
    }
    // Find slot for the cell (from PlayState — but anim engine has no scene
    // context). Solution: animation is purely cosmetic; let scene_play's
    // render() handle the visual since it runs every frame.
    // The animation's only job is to GATE input for the duration.
    active.frame++;
}
```

Actually, for Plan B the simplest correct implementation matches the design spec §9: SELECT_FLASH "Flip cell bg tile to `gb-3` then back". That requires writing to specific tilemap positions, which means anim.c needs to know which cells to flash. The Anim struct has `data[8]` for this.

Since scene_play.c will re-render every frame anyway, and the cell's fill changes based on `selected_mask` (which `toggle_selection` already mutated), the FLASH is essentially a 4-frame visual stutter where the fill alternates. The cleanest implementation:

```c
static void tick_select_flash(void) {
    if (active.frame >= active.duration) {
        active.type = ANIM_NONE;
        return;
    }
    // Visual effect: invert palette every 2 frames for the duration
    if ((active.frame % 2) == 0) {
        BGP_REG = 0x1B;  // inverted palette (00 01 10 11 instead of 11 10 01 00)
    } else {
        BGP_REG = 0xE4;  // normal palette
    }
    active.frame++;
    if (active.frame >= active.duration) {
        BGP_REG = 0xE4;  // ensure restored
    }
}
```

This affects the whole screen, not just one cell, but for a 4-frame flash it's a punchy visual that doesn't require knowing cell coordinates. Acceptable Plan B compromise; refine in Plan C if needed.

### Task 5.7: Cursor blink (continuous, not anim-engine driven)

**Files:**
- Modify: `src/game/scene_play.c`

The design spec §9 lists `ANIM_CURSOR_BLINK` as "always in PLAY, continuous, toggles every 30 frames, toggles sprite OAM visibility bit". This doesn't fit the one-shot anim engine pattern (which is one-active-anim, fire-and-forget). Implement it directly in scene_play.c as a per-frame check using the global_frame_count counter.

- [ ] **Step 1: Add blink logic to play_update**

Add near the top of play_update, alongside the elapsed_seconds increment (added in Phase 6):

```c
    // Cursor blink: toggle sprite visibility every 30 frames
    extern volatile uint16_t global_frame_count;
    if ((global_frame_count / 30) & 1) {
        // Hidden half — move sprite off-screen
        move_sprite(CURSOR_SPRITE_INDEX, 0, 0);
    } else {
        // Visible half — restore at current cursor position
        update_cursor_sprite();
    }
```

For Phase 5 (before global_frame_count exists), this depends on Phase 6 Task 6.1 having added the counter. Either:
- Skip the blink implementation in Phase 5 and add it in Phase 6 alongside the counter setup, OR
- Add a minimal version of `global_frame_count` here in Phase 5 (one extra integer in main.c)

For clarity, add the counter setup as part of Task 5.7 — copy the relevant block from Phase 6 Task 6.1:

In main.c, outside main():

```c
volatile uint16_t global_frame_count = 0;
```

And update the existing `vblank_isr` to increment it:

```c
static void vblank_isr(void) {
    global_frame_count++;
    anim_tick();
    sound_tick();
    render_flush();
}
```

Then the Phase 6 Task 6.1 "Step 1" becomes a no-op (counter already added); Phase 6 only adds the `last_second_frame` tracking in scene_play.c.

### Task 5.8: Build + verify PLAY in mGBA

**Files:** none

- [ ] **Step 1: Build**

```bash
make
```

- [ ] **Step 2: Verify in mGBA**

From TITLE, press A on CONTINUE/NEW GAME → PLAY. Expected:
- Header shows "P1  TRIES:4"
- 16 cells laid out in 2 cols × 8 rows showing words from puzzle 1
- Cursor sprite (hollow square outline) visible on the first cell
- D-pad moves cursor with auto-repeat
- A toggles selection (cell fill changes + screen flashes briefly + SFX)
- After 4 selections, pressing A on a 5th cell plays reject SFX without changing state
- B clears all selections
- SELECT shows quit confirm overlay; A on confirm returns to TITLE (in-progress state saved); B cancels overlay

### Task 5.9: Commit Phase 5

- [ ] **Step 1: Stage and commit**

```bash
git add src/main.c src/game/scene_play.c src/engine/anim.c
git commit -m "B5: SCENE_PLAY rendering + cursor + SELECT_FLASH + CURSOR_BLINK + global frame counter"
```

- [ ] **Step 2: Update Phase 5 banner above**

---

## Phase 6 — SCENE_PLAY submission + animations + transitions

**Execution Status:** ⬜ NOT STARTED

**Goal**: Wire up START → submission logic. On START with 4 selected: call `is_group_correct`; if correct, trigger CORRECT_FLASH → LAYOUT_REFLOW, set group bit, save, possibly transition to WIN; if wrong, trigger CELL_FLASH → WRONG_SHAKE, decrement tries, save, possibly transition to LOSE. Also: drive `elapsed_seconds` from a 60Hz VBlank counter. End state: the game is fully playable — user can solve a complete puzzle from start to finish, transitioning correctly to WIN or LOSE.

### Task 6.1: Add elapsed_seconds tracking in scene_play

**Files:**
- Modify: `src/game/scene_play.c`

The `global_frame_count` counter in main.c was added in Phase 5 Task 5.7 (for CURSOR_BLINK). This task just adds the per-scene second-tick logic.

- [ ] **Step 1: Add tracking state to scene_play.c**

Add to scene_play.c:

```c
static uint16_t last_second_frame = 0;
```

Update play_init to capture the starting frame:

```c
    last_second_frame = global_frame_count;
```

Add to play_update (top of function, before all the conditionals):

```c
    if ((uint16_t)(global_frame_count - last_second_frame) >= 60) {
        ps.elapsed_seconds++;
        last_second_frame += 60;
    }
```

Declare the extern at the top of scene_play.c:

```c
extern volatile uint16_t global_frame_count;
```

### Task 6.2: Implement submission logic

**Files:**
- Modify: `src/game/scene_play.c`

- [ ] **Step 1: Add submission handler**

Replace the Phase 5 stub at the bottom of play_update (`if (input_pressed(BTN_START))`) with:

```c
    if (input_pressed(BTN_START)) {
        if (count_selected(ps.selected_mask) != 4) {
            sfx_reject();
        } else {
            const Puzzle *puzzle = &PUZZLES[pg_save.current_puzzle_index];
            if (is_group_correct(puzzle, ps.selected_mask)) {
                // Find which group was just solved
                uint8_t solved_group = 255;
                for (uint8_t i = 0; i < 16; i++) {
                    if (ps.selected_mask & (1u << i)) {
                        solved_group = find_group_of_word(puzzle, i);
                        break;
                    }
                }

                ps.groups_solved |= (uint8_t)(1u << solved_group);
                ps.selected_mask = 0;

                uint8_t data[8] = { solved_group };
                anim_start(ANIM_CORRECT_FLASH, data, 12);
                sfx_correct();

                // Save progress
                pg_save.ip_tries_remaining = ps.tries_remaining;
                pg_save.ip_groups_solved   = ps.groups_solved;
                pg_save.ip_selected_mask   = 0;
                pg_save.ip_elapsed_seconds = ps.elapsed_seconds;
                save_store(&pg_save);

                // Recompute layout for the reflow
                compute_play_layout(ps.groups_solved, &pg_layout);

                // Check for full solve → WIN
                if (ps.groups_solved == 0x0F) {
                    // All 4 groups solved — update lifetime stats + transition to WIN.
                    // Save changes per spec §4 table:
                    pg_save.puzzles_solved_total++;
                    pg_save.current_streak++;
                    if (pg_save.current_streak > pg_save.best_streak) {
                        pg_save.best_streak = pg_save.current_streak;
                    }
                    pg_save.total_tries_used += (uint16_t)(4 - ps.tries_remaining);
                    pg_save.current_puzzle_index++;
                    pg_save.current_puzzle_fails = 0;
                    // Clear in-progress
                    pg_save.ip_tries_remaining = 0;
                    pg_save.ip_groups_solved   = 0;
                    pg_save.ip_selected_mask   = 0;
                    pg_save.ip_elapsed_seconds = 0;
                    save_store(&pg_save);
                    *next_scene = SCENE_WIN;
                }
            } else {
                // Wrong group
                ps.tries_remaining--;
                uint8_t data[8] = {
                    (uint8_t)(ps.selected_mask & 0xFF),
                    (uint8_t)((ps.selected_mask >> 8) & 0xFF)
                };
                anim_start(ANIM_CELL_FLASH, data, 12);
                sfx_wrong();

                pg_save.ip_tries_remaining = ps.tries_remaining;
                pg_save.ip_groups_solved   = ps.groups_solved;
                pg_save.ip_selected_mask   = ps.selected_mask;
                pg_save.ip_elapsed_seconds = ps.elapsed_seconds;
                save_store(&pg_save);

                if (ps.tries_remaining == 0) {
                    // Spec §4 table: PLAY→LOSE
                    pg_save.current_puzzle_fails++;
                    pg_save.ip_tries_remaining = 0;
                    pg_save.ip_groups_solved   = 0;
                    pg_save.ip_selected_mask   = 0;
                    pg_save.ip_elapsed_seconds = 0;
                    save_store(&pg_save);
                    *next_scene = SCENE_LOSE;
                }
            }
        }
    }
```

### Task 6.3: Real CELL_FLASH + CORRECT_FLASH + LAYOUT_REFLOW

**Files:**
- Modify: `src/engine/anim.c`

The Plan A stubs just advance frame + end. Real implementations:

- [ ] **Step 1: Implement CELL_FLASH (palette inversion on selected cells)**

Replace the generic stub case for ANIM_CELL_FLASH:

```c
static void tick_cell_flash(void) {
    // Check end/chain condition FIRST (before incrementing frame), so the
    // chain transition fires on the same tick that the duration expires.
    // If we incremented first and checked equality after, the chain would
    // be unreachable because the early `>= duration` block on entry would
    // fire on the next tick and short-circuit to ANIM_NONE.
    if (active.frame >= active.duration) {
        // CELL_FLASH done — chain to WRONG_SHAKE
        BGP_REG = 0xE4;
        active.type = ANIM_WRONG_SHAKE;
        active.frame = 0;
        active.duration = 6;
        return;
    }
    // 12 frames = 2 cycles of (3 frames inverted, 3 frames normal)
    if ((active.frame / 3) % 2 == 0) {
        BGP_REG = 0x1B;  // inverted: shade 3↔0, shade 2↔1
    } else {
        BGP_REG = 0xE4;
    }
    active.frame++;
}
```

- [ ] **Step 2: Implement CORRECT_FLASH**

```c
static void tick_correct_flash(void) {
    // Same pattern as tick_cell_flash — chain on entry-time check, before
    // the increment, so the chain transition is reachable.
    if (active.frame >= active.duration) {
        // CORRECT_FLASH done — chain to LAYOUT_REFLOW
        BGP_REG = 0xE4;
        active.type = ANIM_LAYOUT_REFLOW;
        active.frame = 0;
        active.duration = 1;  // 1-frame snap
        return;
    }
    if ((active.frame / 3) % 2 == 0) {
        BGP_REG = 0x1B;
    } else {
        BGP_REG = 0xE4;
    }
    active.frame++;
}
```

- [ ] **Step 3: Update the dispatch in anim_tick**

In the switch statement, replace:

```c
case ANIM_CELL_FLASH:
case ANIM_CORRECT_FLASH:
case ANIM_LAYOUT_REFLOW:
case ANIM_BAR_CASCADE:
case ANIM_STATS_FADE:
case ANIM_LOSE_REVEAL:
    tick_generic_stub();
    break;
```

with:

```c
case ANIM_CELL_FLASH:     tick_cell_flash(); break;
case ANIM_CORRECT_FLASH:  tick_correct_flash(); break;
case ANIM_LAYOUT_REFLOW:  tick_generic_stub(); break;  // 1-frame snap; scene_play's render handles the reflow on its next frame
case ANIM_BAR_CASCADE:
case ANIM_STATS_FADE:
case ANIM_LOSE_REVEAL:
    tick_generic_stub();  // remain stubs until Phases 7-8
    break;
```

### Task 6.4: Fix layout-aware cell mapping in play_render

**Files:**
- Modify: `src/game/scene_play.c`

When groups_solved > 0, the layout has fewer cells (`pg_layout.cells_count < 16`) and the mapping from `cell_idx` (0..15) to layout slot is no longer identity. We need to skip cells whose group is already solved.

- [ ] **Step 1: Replace render_cell's TODO with the real slot calculation**

```c
static uint8_t cell_idx_to_slot(const Puzzle *puzzle, uint8_t cell_idx) {
    // Walk cells 0..cell_idx counting non-solved ones to find the slot.
    uint8_t slot = 0;
    for (uint8_t i = 0; i < cell_idx; i++) {
        uint8_t g = find_group_of_word(puzzle, i);
        if (!(ps.groups_solved & (1u << g))) slot++;
    }
    return slot;
}

static void render_cell(const Puzzle *puzzle, uint8_t cell_idx) {
    uint8_t slot = cell_idx_to_slot(puzzle, cell_idx);
    if (slot >= pg_layout.cells_count) return;  // safety

    uint8_t x = pg_layout.cell_x[slot];
    uint8_t y = pg_layout.cell_y[slot];
    uint8_t selected = (ps.selected_mask & (1u << cell_idx)) != 0;

    uint8_t fill = selected ? UI_TILE_FILL_SEL : UI_TILE_FILL_LIGHT;
    for (uint8_t dy = 0; dy < pg_layout.cell_h; dy++) {
        for (uint8_t dx = 0; dx < 9; dx++) {
            render_set_tile(x + dx, y + dy, fill);
        }
    }

    const char *word = puzzle->words[cell_idx];
    uint8_t word_len = 0;
    while (word[word_len] && word_len < 9) word_len++;
    uint8_t text_x = x + (uint8_t)((9 - word_len) / 2);
    uint8_t text_y = y + (uint8_t)(pg_layout.cell_h / 2);
    render_text(text_x, text_y, word);
}
```

### Task 6.5: Render solved bars

**Files:**
- Modify: `src/game/scene_play.c`

- [ ] **Step 1: Add bar rendering to play_render**

Insert at the start of play_render, after the header, before the cell loop:

```c
    // Draw solved bars at top
    for (uint8_t i = 0; i < pg_layout.bars_count; i++) {
        uint8_t tier = pg_layout.bar_tier[i];
        uint8_t y = pg_layout.bar_y[i] + 1;  // +1 because row 0 is header
        // Bar layout: [3 pattern tiles] [10 label tiles with category name] [3 pattern tiles]
        for (uint8_t x = 0; x < 3; x++) {
            render_set_tile(x, y, (uint8_t)(UI_TILE_PATTERN_BASE + tier));
        }
        for (uint8_t x = 17; x < 20; x++) {
            render_set_tile(x, y, (uint8_t)(UI_TILE_PATTERN_BASE + tier));
        }
        // Label background: solid dark
        for (uint8_t x = 3; x < 17; x++) {
            render_set_tile(x, y, UI_TILE_SOLID_DARK);
        }
        // Category name overlays the label (uses font tiles, light-on-dark)
        const Puzzle *puzzle = &PUZZLES[pg_save.current_puzzle_index];
        render_text(4, y, puzzle->category_names[tier]);
    }
```

**Note about text on dark backgrounds:** the font tiles draw glyph pixels in palette index 3 (darkest) on background index 0 (lightest). When overlaid on the solid dark tile (also palette index 3), the glyph will be invisible — both are the same shade. The cheap fix for Plan B: skip rendering the category name on bars, just rely on the tier pattern + position to identify groups. Plan C can ship inverted font tiles.

**Update**: actually drop the `render_text` call:

```c
        // Skipping category name overlay — font tiles render dark-on-light,
        // would be invisible on the dark label background. Plan C will add
        // inverted font tiles for this case.
        // render_text(4, y, puzzle->category_names[tier]);
```

### Task 6.6: Build + verify the full PLAY loop

**Files:** none

- [ ] **Step 1: Build**

```bash
make
```

- [ ] **Step 2: Test correct + wrong submissions**

In mGBA, start a NEW GAME or CONTINUE. Test:
- Select 4 cells of the same group, press START. Expected: screen flashes (CORRECT_FLASH), layout reflows, solved bar appears at top, tries count unchanged, save persists across reset.
- Select 3 cells of one group + 1 cell of another, press START. Expected: screen flashes differently (CELL_FLASH) + shakes (WRONG_SHAKE), tries decrements, selection clears.
- Continue until all 4 groups solved. Expected: transitions to WIN (stub from Phase 1 still says "SCENE WIN").
- Or fail 4 times. Expected: transitions to LOSE (stub from Phase 1).

If layout breaks after solving a group, verify Task 6.4's `cell_idx_to_slot` function.

### Task 6.7: Commit Phase 6

- [ ] **Step 1: Stage and commit**

```bash
git add src/main.c src/game/scene_play.c src/engine/anim.c
git commit -m "B6: SCENE_PLAY submission + CELL_FLASH/CORRECT_FLASH + transitions to WIN/LOSE"
```

- [ ] **Step 2: Update Phase 6 banner above**

---

## Phase 7 — SCENE_WIN

**Execution Status:** ⬜ NOT STARTED

**Goal**: Real WIN scene with BAR_CASCADE animation (4 tier bars fade in via palette ramp, 20 frames each) + stats overlay (tries used, time, attempt) + STATS_FADE animation. START advances to next puzzle (or ALL_DONE if this was the last). SELECT returns to TITLE.

### Task 7.1: Implement scene_win

**Files:**
- Modify: `src/game/scene_win.c`

- [ ] **Step 1: Replace stub with real WIN scene**

```c
#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include "../engine/anim.h"
#include <stdio.h>

static GameSave win_save;
static uint16_t win_elapsed_seconds;   // captured from PLAY's last state
static uint8_t  win_tries_used;
static uint8_t  cascade_step;          // 0..4 — current bar being revealed
static uint16_t cascade_frame;         // frames since cascade_step changed

static void win_init(void) {
    save_load(&win_save);
    // win_save.current_puzzle_index is already pointing AT the next puzzle
    // because PLAY→WIN incremented it. We want stats for the PREVIOUS puzzle.
    // PlayState's elapsed_seconds was cleared from save; reconstruct via the
    // total_tries_used delta. Actually, we don't have the per-puzzle history
    // easily. Simpler: just show NUM_PUZZLES_SOLVED stats from save.
    // (Plan C polish: capture stats more precisely via PlayState→WinState handoff)
    win_elapsed_seconds = 0;  // placeholder — Plan C will capture this properly
    win_tries_used = 0;       // placeholder
    cascade_step = 0;
    cascade_frame = 0;
    sfx_win();
    // Trigger bar cascade
    anim_start(ANIM_BAR_CASCADE, 0, 80);  // 4 bars × 20 frames each
}

static void win_render(void) {
    render_clear();
    render_text(6, 1, "SOLVED!");

    // Reveal bars as cascade progresses
    for (uint8_t i = 0; i < 4 && i < cascade_step; i++) {
        uint8_t y = (uint8_t)(3 + i);
        for (uint8_t x = 0; x < 3; x++) render_set_tile(x, y, (uint8_t)(UI_TILE_PATTERN_BASE + i));
        for (uint8_t x = 17; x < 20; x++) render_set_tile(x, y, (uint8_t)(UI_TILE_PATTERN_BASE + i));
        for (uint8_t x = 3; x < 17; x++) render_set_tile(x, y, UI_TILE_SOLID_DARK);
        // Skip category name overlay (dark-on-dark would be invisible)
    }

    if (cascade_step >= 4) {
        // Stats
        char buf[21];
        sprintf(buf, "PUZZLE %d DONE", win_save.current_puzzle_index);  // already incremented
        render_text(2, 9, buf);
        sprintf(buf, "STREAK: %d", win_save.current_streak);
        render_text(2, 11, buf);
        sprintf(buf, "BEST:   %d", win_save.best_streak);
        render_text(2, 12, buf);

        render_text(2, 15, "START  NEXT");
        render_text(2, 16, "SELECT TITLE");
    }
}

static void win_update(Scene *next_scene) {
    // Advance cascade step every 20 frames during the BAR_CASCADE anim
    if (anim_current() == ANIM_BAR_CASCADE) {
        cascade_frame++;
        if (cascade_frame >= 20 && cascade_step < 4) {
            cascade_step++;
            cascade_frame = 0;
        }
        return;  // input locked during cascade
    }

    if (anim_is_playing()) return;

    if (input_pressed(BTN_START)) {
        // Next puzzle, unless we just finished the last
        if (win_save.current_puzzle_index >= NUM_PUZZLES) {
            *next_scene = SCENE_ALL_DONE;
        } else {
            *next_scene = SCENE_PLAY;
        }
    } else if (input_pressed(BTN_SELECT)) {
        *next_scene = SCENE_TITLE;
    }
}

static void win_teardown(void) { /* nothing */ }

const SceneVTable SCENE_WIN_VTABLE = {
    .init = win_init,
    .update = win_update,
    .render = win_render,
    .teardown = win_teardown,
};
```

### Task 7.2: Real BAR_CASCADE + STATS_FADE in anim.c

**Files:**
- Modify: `src/engine/anim.c`

- [ ] **Step 1: Implement BAR_CASCADE (palette ramp during cascade)**

Replace the generic stub case:

```c
static void tick_bar_cascade(void) {
    if (active.frame >= active.duration) {
        BGP_REG = 0xE4;
        active.type = ANIM_NONE;
        return;
    }
    // 80 frames total. We don't ramp palette here — scene_win.c handles
    // the visual reveal via cascade_step. The animation's role is just to
    // gate input + provide the 80-frame countdown.
    active.frame++;
}
```

- [ ] **Step 2: Update dispatch in anim_tick**

Add to switch:

```c
case ANIM_BAR_CASCADE:    tick_bar_cascade(); break;
case ANIM_STATS_FADE:     tick_generic_stub(); break;  // visual handled by scene_win's render
```

(`STATS_FADE` is still a stub — we can polish in Plan C.)

### Task 7.3: Build + verify WIN

**Files:** none

- [ ] **Step 1: Build, play to win**

```bash
make
```

In mGBA:
- Start a puzzle, solve all 4 groups deliberately (look at PUZZLES[0] data via mGBA memory viewer, or recall the test puzzle structure).
- Watch the cascade reveal happen (4 bars appear one at a time over ~1.3 seconds).
- Verify stats display after cascade completes.
- Press START → goes to PLAY (next puzzle) or ALL_DONE (if last).
- Press SELECT → goes back to TITLE.

If the cascade looks wrong (e.g., bars never appear), check Task 7.1's `cascade_step` increment logic in `win_update`.

### Task 7.4: Commit Phase 7

- [ ] **Step 1: Stage and commit**

```bash
git add src/game/scene_win.c src/engine/anim.c
git commit -m "B7: SCENE_WIN — BAR_CASCADE + stats display + next-puzzle/title flow"
```

- [ ] **Step 2: Update Phase 7 banner above**

---

## Phase 8 — SCENE_LOSE

**Execution Status:** ⬜ NOT STARTED

**Goal**: Real LOSE scene with reveal animation (LOSE_REVEAL, simpler than BAR_CASCADE) + correct grouping display + retry/skip menu (skip option only when `current_puzzle_fails >= 3`). A/START confirms menu selection. SELECT → TITLE.

### Task 8.1: Implement scene_lose

**Files:**
- Modify: `src/game/scene_lose.c`

- [ ] **Step 1: Replace stub with real LOSE scene**

```c
#include "scene.h"
#include "game_state.h"
#include "puzzles_types.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include "../engine/anim.h"
#include <stdio.h>

typedef enum {
    LOSE_MENU_RETRY = 0,
    LOSE_MENU_SKIP,
    LOSE_MENU_COUNT
} LoseMenuItem;

static GameSave lose_save;
static uint8_t  lose_menu_cursor;
static bool     lose_skip_available;
static uint8_t  reveal_step;
static uint16_t reveal_frame;

static void lose_init(void) {
    save_load(&lose_save);
    lose_menu_cursor = 0;
    lose_skip_available = (lose_save.current_puzzle_fails >= 3);
    reveal_step = 0;
    reveal_frame = 0;
    sfx_lose();
    anim_start(ANIM_LOSE_REVEAL, 0, 80);
}

static void lose_render(void) {
    render_clear();
    render_text(3, 1, "OUT OF TRIES");

    // Reveal the correct groupings progressively
    for (uint8_t i = 0; i < 4 && i < reveal_step; i++) {
        uint8_t y = (uint8_t)(3 + i);
        for (uint8_t x = 0; x < 3; x++) render_set_tile(x, y, (uint8_t)(UI_TILE_PATTERN_BASE + i));
        for (uint8_t x = 17; x < 20; x++) render_set_tile(x, y, (uint8_t)(UI_TILE_PATTERN_BASE + i));
        for (uint8_t x = 3; x < 17; x++) render_set_tile(x, y, UI_TILE_SOLID_DARK);
    }

    if (reveal_step >= 4) {
        char buf[21];
        sprintf(buf, "ATTEMPT: %d", lose_save.current_puzzle_fails);
        render_text(2, 10, buf);

        // Menu (RETRY always; SKIP only if fails >= 3)
        render_text(2, 12, (lose_menu_cursor == 0 ? ">RETRY PUZZLE" : " RETRY PUZZLE"));
        if (lose_skip_available) {
            render_text(2, 13, (lose_menu_cursor == 1 ? ">SKIP PUZZLE" : " SKIP PUZZLE"));
        }

        render_text(2, 16, "SELECT TITLE");
    }
}

static void lose_update(Scene *next_scene) {
    if (anim_current() == ANIM_LOSE_REVEAL) {
        reveal_frame++;
        if (reveal_frame >= 20 && reveal_step < 4) {
            reveal_step++;
            reveal_frame = 0;
        }
        return;
    }
    if (anim_is_playing()) return;

    uint8_t menu_count = (lose_skip_available ? 2 : 1);

    if (input_repeat(BTN_UP) && menu_count > 1) {
        lose_menu_cursor = (uint8_t)((lose_menu_cursor + menu_count - 1) % menu_count);
        sfx_move();
    }
    if (input_repeat(BTN_DOWN) && menu_count > 1) {
        lose_menu_cursor = (uint8_t)((lose_menu_cursor + 1) % menu_count);
        sfx_move();
    }

    if (input_pressed(BTN_A) || input_pressed(BTN_START)) {
        if (lose_menu_cursor == 0) {
            // RETRY — no save change needed (ip_* already cleared on PLAY→LOSE)
            sfx_select();
            *next_scene = SCENE_PLAY;
        } else {
            // SKIP — advance puzzle, reset streak, increment skip count
            lose_save.current_puzzle_index++;
            lose_save.current_streak = 0;
            lose_save.puzzles_skipped_total++;
            lose_save.current_puzzle_fails = 0;
            save_store(&lose_save);
            sfx_skip();
            *next_scene = SCENE_PLAY;
        }
    } else if (input_pressed(BTN_SELECT)) {
        *next_scene = SCENE_TITLE;
    }
}

static void lose_teardown(void) { /* nothing */ }

const SceneVTable SCENE_LOSE_VTABLE = {
    .init = lose_init,
    .update = lose_update,
    .render = lose_render,
    .teardown = lose_teardown,
};
```

### Task 8.2: Update LOSE_REVEAL stub in anim.c

**Files:**
- Modify: `src/engine/anim.c`

- [ ] **Step 1: Same pattern as BAR_CASCADE — visual driven by scene, anim just gates input**

Replace the stub case:

```c
case ANIM_LOSE_REVEAL:    tick_generic_stub(); break;  // visual driven by scene_lose
```

(Already handled by `tick_generic_stub`; no code change needed beyond confirming the dispatch is correct.)

### Task 8.3: Build + verify LOSE

**Files:** none

- [ ] **Step 1: Build + force a loss**

```bash
make
```

In mGBA: start a puzzle, submit 4 wrong groups deliberately to drain tries to 0. Expected:
- Transition to LOSE scene
- "OUT OF TRIES" header
- Reveal animation (4 bars appear over ~1.3s)
- ATTEMPT counter shows `current_puzzle_fails` (which should be 1 on first loss)
- Menu shows only RETRY (no SKIP yet)
- A → goes back to PLAY (same puzzle, fresh attempt)

After failing the same puzzle 3 times total:
- LOSE scene shows BOTH RETRY and SKIP menu items
- UP/DOWN navigates between them
- A on SKIP advances to next puzzle, resets streak, increments skipped_total

### Task 8.4: Commit Phase 8

- [ ] **Step 1: Stage and commit**

```bash
git add src/game/scene_lose.c src/engine/anim.c
git commit -m "B8: SCENE_LOSE — LOSE_REVEAL + retry/skip menu (skip gated on fails>=3)"
```

- [ ] **Step 2: Update Phase 8 banner above**

---

## Phase 9 — SCENE_ALL_DONE + final size verification

**Execution Status:** ⬜ NOT STARTED

**Goal**: Final scene shows lifetime totals when the player completes all NUM_PUZZLES puzzles. START → TITLE with cycle restart (`current_puzzle_index = 0`, but lifetime stats preserved). Then a final size + smoke check of the complete ROM. End state: Plan B shipped — a fully playable game from boot to all-done.

### Task 9.1: Implement scene_all_done

**Files:**
- Modify: `src/game/scene_all_done.c`

- [ ] **Step 1: Replace stub with real ALL_DONE scene**

```c
#include "scene.h"
#include "game_state.h"
#include "../engine/render.h"
#include "../engine/input.h"
#include "../engine/save.h"
#include "../engine/sound.h"
#include <stdio.h>

static GameSave done_save;

static void done_init(void) {
    save_load(&done_save);
    sfx_win();  // celebrate
}

static void done_render(void) {
    render_clear();

    char buf[21];
    sprintf(buf, "YOU SOLVED ALL %d!", NUM_PUZZLES);
    render_text(1, 1, buf);

    sprintf(buf, "PUZZLES SOLVED: %d", done_save.puzzles_solved_total);
    render_text(2, 5, buf);
    sprintf(buf, "PUZZLES SKIPPED: %d", done_save.puzzles_skipped_total);
    render_text(2, 6, buf);
    sprintf(buf, "BEST STREAK: %d", done_save.best_streak);
    render_text(2, 7, buf);

    // Average tries — integer math to avoid float
    if (done_save.puzzles_solved_total > 0) {
        uint16_t avg_x10 = (uint16_t)((done_save.total_tries_used * 10) / done_save.puzzles_solved_total);
        sprintf(buf, "AVG TRIES: %d.%d", avg_x10 / 10, avg_x10 % 10);
        render_text(2, 8, buf);
    }

    render_text(2, 14, "START -> TITLE");
}

static void done_update(Scene *next_scene) {
    if (input_pressed(BTN_START)) {
        // Cycle restart: reset puzzle index to 0, preserve lifetime stats
        done_save.current_puzzle_index = 0;
        done_save.current_puzzle_fails = 0;
        // Lifetime totals preserved
        // In-progress fields are already cleared (no in-progress at ALL_DONE)
        save_store(&done_save);
        sfx_select();
        *next_scene = SCENE_TITLE;
    }
}

static void done_teardown(void) { /* nothing */ }

const SceneVTable SCENE_ALL_DONE_VTABLE = {
    .init = done_init,
    .update = done_update,
    .render = done_render,
    .teardown = done_teardown,
};
```

### Task 9.2: Final ROM size + smoke check

**Files:** none

- [ ] **Step 1: Build + check size**

```bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:/c/gbdk/bin:$PATH" && make clean && make && make size
```

Expected: ROM at 64KB (the MBC1+RAM+BATT header allocates 64KB; actual used bytes will be much less). The `make size` output shows the file size.

If the linker fails with "out of memory" / "section won't fit", we've exceeded the ROM banks. Most likely cause: too much title art. Reduce `make_title.py` output or move title tile loading to dynamic on-demand.

- [ ] **Step 2: Full game playthrough in mGBA**

Play a complete game from TITLE → PLAY → WIN → PLAY → WIN → ... → ALL_DONE. Verify:
- Puzzle index increments each WIN
- Streak increments on solve, resets on skip
- All 5 puzzles play through without crashes
- ALL_DONE shows correct lifetime stats
- START on ALL_DONE returns to TITLE with `current_puzzle_index = 0` but stats preserved
- TITLE menu now shows CONTINUE pointing at puzzle 1 again (cycle restart works)

- [ ] **Step 3: Persistence test**

After playing, hard-reset mGBA, reopen. Verify lifetime stats from the previous session persist via save_load.

### Task 9.3: Update test target verification

**Files:** none

- [ ] **Step 1: Confirm `make test` still passes**

```bash
make test
```

Expected: all 21 C assertions (puzzle_logic) + 13 Python assertions (codegen) + new layout tests pass.

### Task 9.4: Commit Phase 9

- [ ] **Step 1: Stage and commit**

```bash
git add src/game/scene_all_done.c
git commit -m "B9: SCENE_ALL_DONE — lifetime totals + cycle restart"
```

- [ ] **Step 2: Update Phase 9 banner above + mark Plan B complete in top-of-plan table**

---

## End of Plan B

When all 9 phases ship and their banners are updated, Plan B is complete. The artifact at this point:

- A `.gb` ROM that boots to TITLE → adaptive menu (CONTINUE/RESTART/NEW GAME) → PLAY scene → WIN/LOSE → ALL_DONE → cycle restart
- All 5 sample puzzles playable end-to-end with correct save persistence
- All Plan A engine stubs replaced with real animations (SELECT_FLASH, CELL_FLASH, CORRECT_FLASH, BAR_CASCADE, LOSE_REVEAL real; LAYOUT_REFLOW + STATS_FADE remain visual-driven-by-scene stubs which is correct per design)
- 21 + 13 + new layout tests passing via `make test`

**What's deferred to Plan C (Content + Polish):**
- Full 30-puzzle bank (replacing the 5 sample puzzles)
- Title screen art polish (load `title.png`, render the real art instead of text-only)
- Category name overlay on solved bars (requires inverted font tiles)
- STATS_FADE proper palette ramping
- SFX pitch calibration (current values are placeholder estimates per spec §13)
- Hardware verification on real DMG via flash cart
- Cells-only shake fallback if whole-screen SCX shake is too jarring (current implementation)
- ROM-baked SFX/music polish
