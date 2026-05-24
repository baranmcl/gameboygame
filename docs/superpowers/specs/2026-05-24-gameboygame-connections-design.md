# Game Boy Connections — Design Spec

**Date**: 2026-05-24
**Status**: Approved by user, ready for implementation planning
**Target**: Real Game Boy ROM (`.gb` file) playable on emulator and original DMG hardware via flash cart

---

## 1. Overview

A Game Boy ROM recreation of NYT Connections — the player groups 16 words into 4 categories of 4 words each. Built as a learning project for first-time Game Boy development; deliberately constrained scope so the project actually ships.

**The player loop**:
1. See 16 shuffled words on the screen, plus a header with current level and tries remaining (start: 4)
2. Move a cursor with the D-pad, press A to select up to 4 words, press B to clear selection
3. Press START to submit current 4-word selection as a category guess
4. Correct: the group flashes, animates to a solved bar at the top with its tier pattern + category name; remaining cells reflow into freed space
5. Wrong: cells flash, screen briefly shakes, tries decrements
6. Solve all 4 groups → WIN scene with stats overlay; advance to next puzzle
7. Run out of tries → LOSE scene with correct grouping revealed; retry the same puzzle (or skip after 3 fails)

---

## 2. Key decisions (Q&A summary)

| # | Question | Decision |
|---|---|---|
| Q1 | Project kind | Real Game Boy ROM (homebrew), built with GBDK-2020 (C) |
| Q2 | 16-word layout on 160×144 screen | Layout B: 2 columns × 8 rows, standard 8×8 font, up to ~8 chars per word |
| Q3 | Difficulty model | Both puzzle-level ordering (easy→hard) AND within-puzzle color tiers (yellow/green/blue/purple) |
| Q4 | Hardware target | DMG only (original Game Boy + Game Boy Pocket). 4 grey shades + pattern fills for tier distinction |
| Q5 | Save model | MBC1+RAM+Battery. Saves puzzle index, fails count, streak, lifetime totals, in-progress puzzle state |
| Q6 | Puzzle authoring | External `content/puzzles.json` → Python codegen → `src/puzzles_data.c` at build time |
| Q7 | v1 scope | Polished v1: ~30 puzzles, full SFX, animations, per-puzzle stats, save resume, adaptive title menu |
| — | Fail behavior | Block on fail (retry same puzzle); after 3 fails, "SKIP PUZZLE" option appears |
| — | Architecture | Approach B: scene state machine + modular subsystems (engine + game split) |
| — | NEW GAME behavior | Full wipe including lifetime stats |
| — | Solved-group display | NYT-style compacting: solved groups stack as bars at top, unsolved cells reflow |
| — | Wrong-guess feedback | Whole-screen shake via SCX register (v1); cells-only flash fallback if playtesting shows it's jarring |

---

## 3. Architecture & file layout

```
gameboygame/
├── Makefile                  # GBDK build, codegen, asset conversion, ROM packing
├── README.md                 # how to build, play, add puzzles
├── LICENSE
├── .gitignore                # build/, src/puzzles_data.c, src/assets_gen/, .superpowers/
│
├── content/
│   └── puzzles.json          # human-authored puzzle bank
│
├── tools/
│   ├── build_puzzles.py      # JSON → src/puzzles_data.c codegen + validator
│   └── test_build_puzzles.py # unittests for the codegen validator
│
├── assets/                   # raw source art (PNG)
│   ├── font.png              # 8×8 font tile sheet (uppercase + digits + symbols)
│   ├── ui_tiles.png          # cursor, borders, 4 tier-pattern tiles
│   └── title.png             # title screen image
│
├── src/
│   ├── main.c                # entry, top-level loop, VBlank ISR, scene dispatch
│   ├── puzzles_data.c        # GENERATED — packed puzzle data, not in git
│   ├── assets_gen/           # GENERATED — png2asset output, not in git
│   │
│   ├── engine/               # platform-facing subsystems (touch GBDK APIs)
│   │   ├── input.h, input.c          # debounced button state per frame
│   │   ├── render.h, render.c        # tilemap buffer, text drawing, VBlank flush
│   │   ├── sound.h, sound.c          # SFX triggers via GB sound registers
│   │   ├── save.h, save.c            # SRAM read/write, magic + version + checksum
│   │   └── anim.h, anim.c            # animation engine (one active animation at a time)
│   │
│   └── game/                 # game-facing logic (platform-independent, host-testable)
│       ├── game_state.h              # central GameState struct + Scene enum
│       ├── puzzles_types.h           # Puzzle struct typedef (referenced by puzzles_data.c)
│       ├── puzzle_logic.h, .c        # pure functions: is_group_correct, toggle_selection, etc.
│       ├── scene_title.c             # SCENE_TITLE handlers
│       ├── scene_play.c              # SCENE_PLAY handlers (main puzzle screen)
│       ├── scene_win.c               # SCENE_WIN handlers
│       ├── scene_lose.c              # SCENE_LOSE handlers
│       └── scene_all_done.c          # SCENE_ALL_DONE handlers
│
├── test/                     # host-machine tests (vanilla gcc, not GBDK)
│   ├── Makefile              # `make test` — runs on dev machine
│   ├── test_puzzle_logic.c   # unit tests for puzzle_logic.c (no GBDK deps)
│   └── manual_checklist.md   # per-scene smoke test list for emulator
│
├── build/                    # .o files, gameboygame.gb output (gitignored)
│
└── docs/
    └── superpowers/specs/
        └── 2026-05-24-gameboygame-connections-design.md   # this document
```

### Module responsibilities

| Module | Owns | Calls into |
|---|---|---|
| `main.c` | Top-level loop, VBlank ISR, scene dispatch table | All engine + scenes |
| `engine/input` | Reading + debouncing buttons; `input_pressed`, `input_held`, `input_repeat`, `input_released` | GBDK `joypad()` |
| `engine/render` | 20×18 tilemap buffer in WRAM; `render_text`, `render_flush` (VBlank) | GBDK `set_bkg_tiles`, `set_sprite_tile` |
| `engine/sound` | `sfx_move`, `sfx_select`, `sfx_correct`, `sfx_wrong`, `sfx_win`, `sfx_lose`, `sfx_reject`, `sfx_skip`, `sfx_deselect` | Raw `NR10`..`NR52` registers |
| `engine/save` | `save_load`, `save_store`, `save_reset` with magic + version + checksum validation | GBDK `ENABLE_RAM_MBC1` / `DISABLE_RAM_MBC1` |
| `engine/anim` | `anim_start`, `anim_tick`, `anim_is_playing` — one active animation at a time | engine/render |
| `game/puzzle_logic` | Pure functions on Puzzle + PlayState structs | Nothing (no GBDK includes — host-testable) |
| `game/scene_*` | Per-scene `init`/`update`/`render`/`teardown` | engine + puzzle_logic |
| `puzzles_data.c` *(gen)* | `const Puzzle PUZZLES[NUM_PUZZLES]` packed data | Referenced by scenes via `puzzles_types.h` |

### Architectural rules

1. **`game/` files never include GBDK headers directly** — they only touch engine APIs. This keeps puzzle logic host-testable via vanilla gcc.
2. **Rendering is always deferred** — scenes write to the tilemap buffer in their `render()`; only `engine/render`'s VBlank-synced `flush()` touches VRAM.
3. **One scene active at a time** — `scene_*.teardown()` runs before the next `scene_*.init()`, so each scene starts with a clean VRAM.
4. **`puzzles_data.c` is generated, not in git** — `make` regenerates it; prevents merge conflicts and forces validation on every build.
5. **One animation active at a time** — scenes check `anim_is_playing()` before processing input; input is locked during animations.

### Build pipeline (Make)

```
make           →  1. python tools/build_puzzles.py content/puzzles.json src/puzzles_data.c
                  2. png2asset assets/font.png -o src/assets_gen/font.c     (etc.)
                  3. lcc -c src/**/*.c → build/*.o
                  4. lcc -Wl-yt3 -Wl-yo4 -Wl-ya1 -Wm-yc → build/gameboygame.gb
                       (-yt3 = MBC1+RAM+Battery, -yo4 = 4 ROM banks (64KB),
                        -ya1 = 1 RAM bank (8KB), -yc = GBC compatibility off)
make test      →  vanilla gcc test/test_puzzle_logic.c src/game/puzzle_logic.c → ./test_puzzle_logic
                  python -m unittest tools/test_build_puzzles.py
make run       →  make && bgb build/gameboygame.gb   (or sameboy)
make size      →  print ROM size, fail if >64KB
make clean     →  rm -rf build/ src/puzzles_data.c src/assets_gen/
```

### Build prerequisites

- **GBDK-2020** (latest release) — `lcc`, `png2asset`, headers
- **Python 3** (no third-party packages)
- **GNU Make**
- **BGB** or **SameBoy** emulator for dev testing
- A **flash cart** (EverDrive GB / EZ-Flash Junior / Krikzz) for hardware verification

---

## 4. Scene state machine

Five scenes. Modal dialogs (NEW GAME confirm, quit-to-title confirm) live as UI-state flags inside their parent scene — not separate scenes.

| Scene | Owns | Inputs handled |
|---|---|---|
| `SCENE_TITLE` | Title art + adaptive menu (CONTINUE / RESTART / NEW GAME, items shown based on in-progress save state). Owns NEW GAME confirm overlay as internal state. | D-pad up/down moves menu cursor; A/START confirms; on NEW GAME, B cancels confirm. |
| `SCENE_PLAY` | Word list, cursor, selection bitmask, tries counter, solved-groups bitmask, elapsed-time counter, quit-confirm flag. | D-pad moves cursor (auto-repeat); A toggles selection (max 4); B clears all selections; START submits current 4; SELECT triggers quit-to-title confirm overlay. |
| `SCENE_WIN` | Solved-group cascade reveal animation, stats overlay (tries used, time, attempt number), "next puzzle" prompt. | START → next puzzle (or `SCENE_ALL_DONE` if last); SELECT → `SCENE_TITLE`. |
| `SCENE_LOSE` | Reveal of correct groupings, "OUT OF TRIES" message, attempt counter, retry/skip menu (skip option only if fails ≥ 3). | D-pad up/down (only when fails ≥ 3); A or START confirms selected option; SELECT → `SCENE_TITLE`. |
| `SCENE_ALL_DONE` | Celebration screen + lifetime totals (puzzles solved, skipped, best streak, avg tries). | START → `SCENE_TITLE` (cycle restarts with `current_puzzle_index = 0`, lifetime totals preserved). |

### Adaptive TITLE menu

```
If save.ip_tries_remaining > 0 (puzzle is in progress):
  ▸ CONTINUE PUZZLE N
    RESTART PUZZLE N
    NEW GAME

If save.ip_tries_remaining == 0 (no in-progress puzzle):
  ▸ CONTINUE
    NEW GAME
```

RESTART clears in-progress state but preserves `current_puzzle_index`, `current_puzzle_fails`, and lifetime stats. Transitions to PLAY with a fresh attempt.

### Transitions

| Trigger | From → To | Save changes |
|---|---|---|
| Boot, save loads valid | (boot) → TITLE | none |
| Boot, save invalid | (boot) → TITLE | `save_reset()` then save |
| A on TITLE CONTINUE | TITLE → PLAY | none |
| A on TITLE RESTART | TITLE → PLAY | clear all `ip_*` fields, save |
| A on TITLE NEW GAME (after confirm) | TITLE → PLAY | `save_reset()`, then save |
| All 4 groups solved | PLAY → WIN | `puzzles_solved_total++`, `current_streak++`, `best_streak = max(...)`, `current_puzzle_index++`, `current_puzzle_fails = 0`, clear `ip_*` |
| Tries = 0 and not all solved | PLAY → LOSE | `current_puzzle_fails++`, clear `ip_*`. Do NOT advance index. Do NOT touch streak. |
| START on WIN, not last puzzle | WIN → PLAY (next puzzle) | none (index already advanced on PLAY→WIN) |
| START on WIN, last puzzle | WIN → ALL_DONE | none |
| SELECT on WIN | WIN → TITLE | none |
| RETRY chosen on LOSE | LOSE → PLAY (same puzzle) | none |
| SKIP chosen on LOSE (only if fails ≥ 3) | LOSE → PLAY (next puzzle) | `current_puzzle_index++`, `current_streak = 0`, `puzzles_skipped_total++`, `current_puzzle_fails = 0` |
| SELECT on LOSE | LOSE → TITLE | none |
| START on ALL_DONE | ALL_DONE → TITLE | `current_puzzle_index = 0` (cycle restart), lifetime totals preserved |

### Save-write triggers (the only places `save_store()` runs)

1. Every group submission in PLAY (correct or wrong) — updates tries, groups_solved, selection state
2. `PLAY → WIN` transition
3. `PLAY → LOSE` transition
4. TITLE NEW GAME confirm — full wipe via `save_reset()`
5. TITLE RESTART confirm — clears `ip_*` only
6. `LOSE → PLAY (skip)` transition

No mid-cursor-movement writes.

---

## 5. Input & cursor model

### Button map per scene

| Scene | UP | DOWN | LEFT | RIGHT | A | B | START | SELECT |
|---|---|---|---|---|---|---|---|---|
| TITLE | menu up | menu down | — | — | confirm menu choice | cancel confirm overlay | confirm menu choice | — |
| PLAY | cursor up | cursor down | cursor left | cursor right | toggle selection (max 4) | clear all selections | submit current group | quit-to-title (with confirm) |
| WIN | — | — | — | — | — | — | next puzzle (or ALL_DONE) | back to title |
| LOSE (fails<3) | — | — | — | — | — | — | retry | back to title |
| LOSE (fails≥3) | menu up | menu down | — | — | confirm menu choice | — | confirm selected option | back to title |
| ALL_DONE | — | — | — | — | — | — | back to title | back to title |

### Cursor navigation rules in SCENE_PLAY (2×8 grid)

- Cells indexed 0..15. `index = row * 2 + col`, `row ∈ [0..7]`, `col ∈ [0..1]`.
- UP: `row = (row - 1 + 8) % 8` (wraps vertically)
- DOWN: `row = (row + 1) % 8` (wraps vertically)
- LEFT: `col = 0` (single-tap snap, no wrap)
- RIGHT: `col = 1` (single-tap snap, no wrap)
- Cursor index stored in `PlayState.cursor_idx` (u8, 0-15).
- D-pad auto-repeat: after ~250ms held (15 frames), repeats every ~80ms (5 frames). Action buttons (A/B/START/SELECT) stay edge-triggered.

### Selection rules

- `selected_mask` is a u16 bitmask of which of 16 word cells are currently selected.
- A: toggle bit at `cursor_idx`. If 4 already selected and attempting to set a 5th, ignore + `sfx_reject()`.
- B: clears entire `selected_mask` in one press (no confirm).
- START: only valid if `popcount(selected_mask) == 4`. Otherwise ignore + `sfx_reject()`.

### PlayState struct

```c
typedef struct {
  uint8_t  cursor_idx;          // 0-15
  uint16_t selected_mask;       // bits 0..15 — currently selected
  uint8_t  groups_solved;       // bits 0..3 — yellow,green,blue,purple
  uint8_t  tries_remaining;     // starts at 4
  uint16_t elapsed_seconds;     // VBlank-driven 60-tick counter, for stats overlay
  uint8_t  show_quit_confirm;   // 0 or 1 — SELECT-triggered "are you sure?" overlay
} PlayState;
```

Persistent subset (everything except `cursor_idx` and `show_quit_confirm`) gets serialized to SRAM.

**PLAY scene init logic**: when SCENE_PLAY is entered, it reads the save's `ip_*` fields. If `save.ip_tries_remaining > 0`, restore in-progress state (tries, groups_solved, selected_mask, elapsed_seconds) from save. If `save.ip_tries_remaining == 0`, this is a fresh attempt — initialize `tries_remaining = 4`, `groups_solved = 0`, `selected_mask = 0`, `elapsed_seconds = 0`. In both cases, `cursor_idx` always resets to 0 (cursor position is never saved). This is why "RETRY chosen on LOSE" needs no explicit save change in §4's transition table — the LOSE transition already cleared `ip_*`, so re-entering PLAY automatically starts fresh.

### Input subsystem API

```c
typedef enum {
  BTN_UP=0x04, BTN_DOWN=0x08, BTN_LEFT=0x02, BTN_RIGHT=0x01,
  BTN_A=0x10,  BTN_B=0x20,    BTN_START=0x80, BTN_SELECT=0x40
} Button;

void input_update(void);              // once per frame, before scene update
bool input_pressed(Button b);         // edge-triggered: pressed this frame
bool input_held(Button b);            // raw: currently held
bool input_repeat(Button b);          // edge-triggered + auto-repeat (D-pad)
bool input_released(Button b);        // edge-triggered: released this frame
```

---

## 6. Puzzle data format & build pipeline

### Source format: `content/puzzles.json`

```json
{
  "puzzles": [
    {
      "id": 1,
      "categories": [
        { "tier": "yellow", "name": "BIRDS",  "words": ["ROBIN", "EAGLE", "HAWK", "FINCH"] },
        { "tier": "green",  "name": "COLORS", "words": ["RED", "BLUE", "GREEN", "BLACK"] },
        { "tier": "blue",   "name": "ACTION", "words": ["RUN", "JUMP", "SWIM", "KICK"] },
        { "tier": "purple", "name": "FOOD",   "words": ["BREAD", "RICE", "PASTA", "MEAT"] }
      ]
    }
  ]
}
```

Puzzles may include `"draft": true` to be excluded from the build (lets authors park half-written puzzles without breaking validation).

### Validation rules (codegen fails build if any are violated)

1. `puzzles` array non-empty; `id` values sequential starting at 1, no gaps among non-draft entries
2. Per puzzle: exactly 4 categories; tiers exactly `{yellow, green, blue, purple}`, no duplicates
3. Per category: exactly 4 words; `name` is A-Z + space, length 1-12
4. Per word: uppercase A-Z only, length 1-8
5. No duplicate words within a puzzle (case-insensitive)
6. No category-name = word collision within a puzzle

Error format: `puzzle 7, category "PASTA": word "SPAGHETTI" exceeds 8-char limit (9 chars)`.

### Generated output: `src/puzzles_data.c` (gitignored)

```c
// AUTO-GENERATED by tools/build_puzzles.py — DO NOT EDIT
#include "game/puzzles_types.h"

const uint8_t NUM_PUZZLES = 30;

const Puzzle PUZZLES[30] = {
  {
    .words = { "PASTA", "ROBIN", "RED", "JUMP", "FINCH", "MEAT", "BREAD",
               "GREEN", "HAWK", "SWIM", "BLACK", "EAGLE", "RICE", "KICK",
               "BLUE", "RUN" },
    .group_of = { 3, 0, 1, 2, 0, 3, 3, 1, 0, 2, 1, 0, 3, 2, 1, 2 },
    .category_names = { "BIRDS", "COLORS", "ACTION", "FOOD" }
  },
  // ...
};
```

### Hand-written struct: `src/game/puzzles_types.h` (checked in)

```c
#ifndef PUZZLES_TYPES_H
#define PUZZLES_TYPES_H

#include <stdint.h>

#define WORDS_PER_PUZZLE     16
#define GROUPS_PER_PUZZLE     4
#define WORDS_PER_GROUP       4
#define MAX_WORD_LEN          8
#define MAX_CATEGORY_NAME_LEN 12

typedef struct {
  char    words[WORDS_PER_PUZZLE][MAX_WORD_LEN + 1];
  uint8_t group_of[WORDS_PER_PUZZLE];
  char    category_names[GROUPS_PER_PUZZLE][MAX_CATEGORY_NAME_LEN + 1];
} Puzzle;

extern const uint8_t NUM_PUZZLES;
extern const Puzzle  PUZZLES[];

#endif
```

### Deterministic shuffle

The Python script shuffles category-grouped JSON into position-ordered C using `puzzle_id` as the seed:

```python
import random
def shuffle_puzzle(puzzle_id, words_with_groups):
    rng = random.Random(puzzle_id * 31 + 7)  # fixed seed formula
    shuffled = list(words_with_groups)
    rng.shuffle(shuffled)
    return shuffled
```

Same seed formula → same shuffled output every build → save-state stability across rebuilds. If we intentionally reshuffle (to fix a UX issue), bumping the formula is an explicit save-breaking change.

### ROM size budget (rough)

| Piece | Estimate |
|---|---|
| 30 puzzles @ ~200 bytes | ~6 KB |
| Engine code | ~4 KB |
| Scenes (5) | ~4 KB |
| Puzzle logic | ~1 KB |
| Font tiles (~64 × 16 bytes) | ~1.5 KB |
| UI tiles (cursor, borders, 4 tier patterns, label bg) | ~0.5 KB |
| Title screen art | ~2 KB |
| Sound data (9 SFX) | ~0.5 KB |
| Cartridge header + padding | ~0.5 KB |
| **Total est.** | **~20 KB** |

Allocated 64 KB (4 ROM banks via MBC1). ~44 KB headroom for content/polish/v1.1 features.

---

## 7. Save format (SRAM layout)

### Byte layout (20 bytes at `0xA000`)

```c
typedef struct {
  uint8_t  magic[4];                 // [0..3]   "GBCX" validity sentinel
  uint8_t  version;                  // [4]      schema version (currently 1)
  uint8_t  current_puzzle_index;     // [5]      0..NUM_PUZZLES-1
  uint8_t  current_puzzle_fails;     // [6]      0..3 (gates skip option)
  uint8_t  puzzles_solved_total;     // [7]      lifetime solved (no skips)
  uint8_t  puzzles_skipped_total;    // [8]      lifetime skipped
  uint8_t  current_streak;           // [9]      consecutive solves without skip
  uint8_t  best_streak;              // [10]     lifetime best streak
  uint16_t total_tries_used;         // [11..12] lifetime tries, for avg calc
  // ----- in-progress puzzle state -----
  uint8_t  ip_tries_remaining;       // [13]     0..4 (0 = no in-progress)
  uint8_t  ip_groups_solved;         // [14]     bits 0..3
  uint16_t ip_selected_mask;         // [15..16] bits 0..15
  uint16_t ip_elapsed_seconds;       // [17..18] continuity for stats overlay
  // ----- integrity -----
  uint8_t  checksum;                 // [19]     XOR of bytes [0..18]
} GameSave;
```

**Sentinel**: `ip_tries_remaining == 0` means "no puzzle in progress" (fresh start, not mid-game). Valid in-progress values are 1..4.

### Magic / version / checksum

- `magic` = `"GBCX"` (GameBoy Connections X)
- `SAVE_VERSION` = 1; future schema changes bump this and trigger migration logic
- Checksum = XOR of all preceding 19 bytes; verified on every `save_load`

### Save API

```c
bool save_load(GameSave *out);       // returns true if valid save existed; resets to defaults if not
bool save_store(const GameSave *in); // writes with fresh checksum, brackets with ENABLE/DISABLE RAM
void save_reset(GameSave *out);      // wipe to factory defaults (NEW GAME)
```

### `save_load` validation flow

```
1. ENABLE_RAM_MBC1
2. Read 20 bytes from 0xA000 into temp buffer
3. DISABLE_RAM_MBC1
4. Validate:
     a) magic == "GBCX"           → if no, invalid
     b) version == SAVE_VERSION    → if no, invalid
     c) checksum matches           → if no, invalid
     d) sanity: current_puzzle_index < NUM_PUZZLES, ip_groups_solved < 0x10
5. If valid: copy to *out, return true
6. If invalid: save_reset(out), save_store(out), return false
```

### Default state (`save_reset`)

```
magic = "GBCX", version = 1, all other bytes = 0, checksum recomputed.
```

---

## 8. Rendering

### Palette (DMG, 4 shades fixed)

| Index | Hex | Use |
|---|---|---|
| 0 | `#9bbc0f` (lightest) | Screen background, default cell bg, solved label text |
| 1 | `#8bac0f` (light) | Unsolved cell fill |
| 2 | `#306230` (dark) | Selected cell fill, cursor highlights |
| 3 | `#0f380f` (darkest) | Text, borders, solved-bar label background |

### Font

- A-Z, 0-9, plus ~20 UI glyphs (cursor arrows, punctuation) → ~64 unique 8×8 tiles
- Hand-pixeled in any pixel art editor → `assets/font.png` → `png2asset` → C tile array

### Tier pattern tiles (4 tiles, 16 bytes each = 64 bytes total)

- **Yellow** (tier 0, easiest): sparse dots on light background
- **Green** (tier 1): horizontal stripes
- **Blue** (tier 2): vertical stripes
- **Purple** (tier 3, hardest): checker

### Solved-bar structure

A horizontal strip of background tiles: `[pattern tiles] [solid darkest-shade label tiles with font glyphs on top] [pattern tiles]`. Label region uses `gb-3` background with `gb-0` text — same on every tier — for maximum legibility regardless of tier pattern intensity. Tier pattern decorates the area around the label and (combined with vertical position in the solved stack) identifies the tier.

### Tile budget (out of 256 unique tiles, worst-case PLAY scene)

| Category | Tiles |
|---|---|
| Font | ~64 |
| UI chrome (cell borders, corners, cursor) | ~12 |
| Tier patterns (4) | 4 |
| Solid label tile | 1 |
| Title screen art | ~40 |
| Margin | ~135 |
| **Used in PLAY** | **~81 / 256** |

### Sprite usage (40 sprites max, 10 per scanline)

- Cursor highlight: 1 sprite (8×8 with thick-border tile, semi-transparent center)
- Reserved for animations: 0 budget (animations use background tile changes + SCX register, not sprites)

### Dynamic compacting layout (PLAY scene)

| Solved groups | Layout |
|---|---|
| 0 | 8 rows × 2 cols of unsolved cells, ~15px per cell |
| 1 | 1 solved bar at top + 6 rows × 2 cols of unsolveds (~17px per cell) |
| 2 | 2 solved bars + 4 rows × 2 cols (~22px per cell) |
| 3 | 3 solved bars + 2 rows × 2 cols (~37px per cell — generous size) |
| 4 | (triggers `PLAY → WIN`) |

Layout reflow is a single-frame snap-redraw after the correct-group flash animation completes.

### Screen mockups (text outlines)

**Title (in-progress puzzle case):**
```
+----------------------+
|   GAMEBOY            |
|   CONNECTIONS        |
|                      |
|   ▸ CONTINUE PUZZLE 3 |
|     RESTART PUZZLE 3  |
|     NEW GAME         |
|                      |
|   solved: 8   best:5 |
+----------------------+
```

**WIN (after cascade animation):**
```
+----------------------+
|       SOLVED!        |
|                      |
| [yellow] BIRDS       |
| [ green] REDS        |
| [ blue ] MOVE        |
| [purple] PASTA       |
|                      |
| TRIES: 3/4           |
| TIME:  1:34          |
| ATTEMPT: 1           |
|                      |
| START: next puzzle   |
| SELECT: title        |
+----------------------+
```

**LOSE (with skip available, fails ≥ 3):**
```
+----------------------+
|    OUT OF TRIES      |
|                      |
| The answers were:    |
| [yellow] BIRDS       |
| [ green] REDS        |
| [ blue ] MOVE        |
| [purple] PASTA       |
|                      |
| ATTEMPT: 3 of ∞      |
|                      |
| ▸ RETRY PUZZLE       |
|   SKIP PUZZLE        |
+----------------------+
```

When `current_puzzle_fails < 3`: the SKIP line is hidden and START retries.

**ALL_DONE:**
```
+----------------------+
|   YOU SOLVED ALL 30! |
|                      |
|   PUZZLES SOLVED: 30 |
|   PUZZLES SKIPPED: 0 |
|   BEST STREAK: 30    |
|   AVG TRIES: 2.7     |
|                      |
|   START → title      |
+----------------------+
```

---

## 9. Animations & SFX

### Animation catalog

| Name | Trigger | Frames | Implementation |
|---|---|---|---|
| `ANIM_CURSOR_BLINK` | always in PLAY | continuous, toggles every 30 frames | Toggle sprite OAM visibility bit |
| `ANIM_SELECT_FLASH` | A pressed (toggle on) | 4 frames | Flip cell bg tile to `gb-3` then back |
| `ANIM_CELL_FLASH` | wrong submission (on 4 selected cells) | 12 frames | Invert palette indices on those 4 cells; 2 flash cycles |
| `ANIM_WRONG_SHAKE` | wrong submission, after CELL_FLASH | 6 frames | Whole-screen ±1px jitter via `SCX` register |
| `ANIM_CORRECT_FLASH` | correct submission | 12 frames | Same as CELL_FLASH but ends in tier-pattern fill |
| `ANIM_LAYOUT_REFLOW` | after CORRECT_FLASH | 1 frame (snap) | Clear tiles below header, redraw with new solved-bar + reflowed unsolveds |
| `ANIM_BAR_CASCADE` | WIN init | 80 frames (20 per bar × 4) | Each tier bar fades in via palette ramp |
| `ANIM_STATS_FADE` | WIN, after BAR_CASCADE | 20 frames | Stats text via 2-stage palette swap |
| `ANIM_LOSE_REVEAL` | LOSE init | 80 frames | Same as BAR_CASCADE, lower intensity (no flash punctuation) |

**One animation at a time.** Input is locked during animations (scenes check `anim_is_playing()` before processing input).

### Animation engine API

```c
typedef enum {
  ANIM_NONE = 0,
  ANIM_SELECT_FLASH, ANIM_CELL_FLASH, ANIM_WRONG_SHAKE,
  ANIM_CORRECT_FLASH, ANIM_LAYOUT_REFLOW,
  ANIM_BAR_CASCADE, ANIM_STATS_FADE, ANIM_LOSE_REVEAL
} AnimType;

typedef struct {
  AnimType type;
  uint16_t frame;
  uint16_t duration;
  uint8_t  data[8];
} Anim;

void anim_start(AnimType type, const uint8_t *data, uint16_t duration);
void anim_tick(void);          // called once per frame from VBlank handler
bool anim_is_playing(void);    // input gating
AnimType anim_current(void);
```

### SFX catalog

| Name | Trigger | Channels | Duration | Sound character |
|---|---|---|---|---|
| `sfx_move()` | cursor moves | CH2 | ~30ms | very short high tick (~2 kHz) |
| `sfx_select()` | A toggles selection ON | CH2 | ~50ms | brief upward tick (~3 kHz) |
| `sfx_deselect()` | A toggles selection OFF | CH2 | ~50ms | brief downward tick (~2 kHz) |
| `sfx_reject()` | A on full selection or START with <4 | CH4 | ~100ms | low noise buzz |
| `sfx_correct()` | submit correct | CH1 | ~240ms | rising 3-note arpeggio (major triad) |
| `sfx_wrong()` | submit wrong | CH1 | ~200ms | descending 2-note "uh-oh" |
| `sfx_win()` | PLAY → WIN | CH1+CH2 | ~600ms | celebratory arpeggio + held resolve |
| `sfx_lose()` | PLAY → LOSE | CH1+CH2 | ~500ms | descending sad chord pair |
| `sfx_skip()` | skip chosen | CH1 | ~200ms | neutral 2-note descent |

### Sound implementation

Each SFX is a hardcoded sequence of `NR10`-`NR52` register writes (CH3 unused in v1). A tiny scheduler (one active SFX at a time, latest call interrupts previous) tracks multi-step SFX. ~150 lines of straight C in `engine/sound.c`.

### Wrong-guess shake — v1 decision

Whole-screen shake via `SCX` register for ~6 frames at ±1px. Simplest implementation (~5 lines of code), classic feel. If playtesting reveals it's too jarring, fall back to cells-only flash (1-line change: skip the WRONG_SHAKE animation). Cells-only sprite-based shake stays a v1.1 polish item.

---

## 10. Testing strategy

| Layer | What | When | Tool |
|---|---|---|---|
| 1. Pure logic unit tests | `puzzle_logic.c` functions | `make test` (every dev build) | vanilla gcc + asserts |
| 2. Codegen validation tests | `tools/build_puzzles.py` | `make test` | Python `unittest` |
| 3. Emulator integration | Full playthroughs of each scene | manual, per feature milestone | BGB / SameBoy |
| 4. Hardware verification | Boot + play on real DMG | once per major milestone | flash cart + Game Boy |

### Layer 1: Pure logic unit tests

`game/puzzle_logic.c` has no GBDK includes — compiled with vanilla gcc on dev machine. Test file structure:

```c
// test/test_puzzle_logic.c
static const Puzzle TEST_PUZZLE = {
  .words = {"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P"},
  .group_of = {0,0,0,0, 1,1,1,1, 2,2,2,2, 3,3,3,3},
  .category_names = {"G0","G1","G2","G3"}
};

static void test_is_group_correct_all_same_group(void) {
  assert(is_group_correct(&TEST_PUZZLE, 0x000F) == true);
}
static void test_is_group_correct_three_plus_one(void) {
  assert(is_group_correct(&TEST_PUZZLE, 0x0017) == false);
}
// ... etc
```

Target coverage: every function in `puzzle_logic.c`, every branch (~80% line coverage).

### Layer 2: Codegen validation tests

Tests for every validation rule in §6:

```python
class TestValidate(unittest.TestCase):
    # one passing case + one failing case per validation rule
    def test_valid_puzzle_passes(self): ...
    def test_id_gap(self): ...                  # rule 1
    def test_missing_tier(self): ...            # rule 2 (tiers wrong)
    def test_duplicate_tier(self): ...          # rule 2 (tiers wrong)
    def test_wrong_category_count(self): ...    # rule 2 (not 4 categories)
    def test_wrong_word_count(self): ...        # rule 3 (not 4 words)
    def test_name_too_long(self): ...           # rule 3 (name format)
    def test_word_too_long(self): ...           # rule 4 (word format)
    def test_word_lowercase(self): ...          # rule 4 (word format)
    def test_duplicate_word(self): ...          # rule 5
    def test_name_word_collision(self): ...     # rule 6
```

Target: 100% of validation rules covered — one passing case + one failing case per rule.

### Layer 3: Emulator integration

`test/manual_checklist.md` — per-scene smoke test list. Run after each milestone in BGB or SameBoy. Verify save bytes via emulator's memory viewer at `0xA000`. Verify tile loading via VRAM Tile Viewer. Test save corruption recovery: manually corrupt one byte, power-cycle, verify defaults restore.

### Layer 4: Hardware verification

Once per milestone, copy `gameboygame.gb` to flash cart, insert into real DMG, verify:
- Audio (real GB has high-frequency noise floor that emulators miss)
- LCD ghosting on fast animations (~30ms pixel response — may need to slow some animations)
- Contrast (real DMG is much lower contrast — tier patterns must remain readable)
- Battery save persistence across 30-second power-off

### What we're NOT testing

- Engine modules in isolation — they touch GBDK hardware; mocking would exceed the modules themselves
- Pixel-perfect scene rendering — visual review instead
- Cycle-accurate timing — we're not perf-bound
- Save migration — v1 is schema version 1; add when shipping v2

### Test investment estimate

~1 day of work total: 15 puzzle_logic tests (~3 hrs), 8 codegen tests (~2 hrs), checklist (~1 hr), hardware per-milestone (~30 min each).

---

## 11. Build & dev workflow

```bash
make test       # host-side unit tests + codegen tests (~1 second)
make            # build ROM (~10 seconds)
make run        # build + launch in BGB emulator
make size       # check ROM size, fail if >64KB
make clean      # remove generated + build artifacts
```

Typical loop: edit puzzle_logic.c → `make test` (1s) → fix → repeat. Or edit a scene → `make run` (~10s) → playtest → repeat.

---

## 12. Out of scope for v1 (deferred to v1.1+)

| Feature | Why deferred |
|---|---|
| Title-screen chiptune music | Adds ~4-8 KB and significant CH1+CH2 scheduling complexity; not core to gameplay |
| Daily-puzzle mode (deterministic puzzle from internal day counter) | Requires a calendar tracking mechanism; deferred until core game proven on hardware |
| Hint system (spend a try to dim one wrong cell) | Adds menu surface and changes scoring; needs playtesting on basic game first |
| Stats history screen (detailed per-puzzle log) | Lifetime totals already shown on TITLE and ALL_DONE; detailed log is incremental polish |
| Cells-only shake (per-scanline `SCX` via HBlank interrupts) | Will use whole-screen shake in v1; switch if playtesting shows it's too jarring |
| Word deduplication across puzzles (shared string pool) | Save ~2KB but adds substantial code complexity; revisit if puzzle count > 100 |
| GBC color enhancement | Was rejected — DMG-only is the chosen target |
| Save migration system | Add when shipping schema version 2 |
| Automated playthrough scripts (BGB Lua) | Manual checklist is sufficient for v1 scope |
| Multiple save slots / per-player profiles | Single-player puzzle game; one slot is fine |

---

## 13. Open questions / decisions to confirm during implementation

None at design time — all major decisions are locked. The following may surface during implementation and warrant a quick discussion if they come up:

- Final timing constants for animations (frame counts) may need tuning based on real DMG LCD response
- Sound frequencies for SFX may need adjustment based on real DMG audio character
- Cell border styling (thickness, corner treatment) may need iteration based on emulator screenshots
- The "ATTEMPT: N of ∞" text in the LOSE screen — final wording subject to revision

---

*End of design spec.*
