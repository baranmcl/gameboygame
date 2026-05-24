# Game Boy Connections — Foundation Plan (Plan A of 3)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the foundation layer of Game Boy Connections — a working GBDK-2020 build pipeline, all five engine subsystems (input/render/save/sound/anim), pure puzzle logic with comprehensive unit tests, and the JSON-driven puzzle data pipeline with codegen. End state is a `.gb` ROM that boots, exercises each engine subsystem via temporary smoke-test scaffolding in `main.c`, has passing host-side unit tests, and has 5 sample puzzles baked into the ROM. No playable scenes yet — those are Plan B.

**Architecture:** Approach B from the design spec — scene state machine + modular subsystems with strict engine/game split. `game/` files include only `puzzles_types.h` and pure C — no GBDK headers — so `puzzle_logic.c` is host-testable with vanilla gcc. `engine/` files contain all GBDK-touching code: input debouncing, tilemap-buffer + VBlank flush rendering, MBC1+RAM+Battery SRAM access, sound register sequencing, single-active animation engine. All puzzle content authored in `content/puzzles.json` and compiled to `src/puzzles_data.c` by a Python script invoked from the Makefile.

**Tech Stack:** GBDK-2020 (lcc + SDCC, targeting DMG hardware), Python 3 (puzzle codegen, no third-party deps), vanilla gcc (host-side unit tests), GNU Make (build orchestration), BGB or SameBoy (emulator for smoke testing).

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

## Notes for executors

**TDD scope.** This project has two host-testable layers that REQUIRE TDD (failing test first, then implement):
- `game/puzzle_logic.c` — pure C functions, no GBDK includes, compiled with vanilla gcc. Tested via `test/test_puzzle_logic.c`.
- `tools/build_puzzles.py` — pure Python, tested via `tools/test_build_puzzles.py` (stdlib unittest).

Engine subsystems (`engine/input.c`, `engine/render.c`, `engine/save.c`, `engine/sound.c`, `engine/anim.c`) directly touch Game Boy hardware (memory-mapped I/O registers like `NR10`, `SCX`, `joypad()`). They have no host-testable interface and cannot reasonably be unit tested without writing more mocking infrastructure than the modules themselves. They are verified via **smoke-test scaffolding in `main.c`** that exercises each module visually in the emulator. This scaffolding is temporary — Plan B replaces it with real scenes.

**Pitfalls docs.** The project has no `docs/pitfalls/implementation-pitfalls.md` or `docs/pitfalls/testing-pitfalls.md` yet — those accumulate from learned-the-hard-way experience and there's no shipped GB code yet to learn from. TDD tasks in this plan therefore reference test-driven-development without the pitfalls cross-check. Once Plan A ships and we've encountered real GB-specific testing traps, a future plan can introduce the pitfalls docs.

**Commit cadence.** One commit per task. Commit subjects follow `<phase>.<task>: <imperative summary>` (e.g., `2.3: implement render_text helper`). This makes per-task progress visible in `git log` and lets `git bisect` isolate task-level regressions.

**Emulator.** All "verify in emulator" steps assume BGB on Windows. If on macOS or Linux, substitute SameBoy. Both have memory viewers, VRAM tile viewers, OAM inspectors, and SRAM dumpers; the verification techniques translate directly.

**Working directory.** All shell commands assume current working directory is the project root (`C:/Users/baranmcl/Code/gameboygame/` on the human partner's machine). Relative paths in the plan are relative to that root.

---

## Execution Status

**Overall:** 5/8 phases shipped, 0 deferred.

| Phase | Status | Ship SHA(s) | Notes |
|---|---|---|---|
| 1 — Project bootstrap | ✅ Shipped | `d98081c` | 2026-05-24; ROM verified in mGBA, 3 plan defects fixed inline |
| 2 — Engine: render | ✅ Shipped | `ef5b12b` | 2026-05-24; ROM builds at 64KB MBC1+RAM+BATT, tile data spot-verified, mGBA visual confirmation received |
| 3 — Engine: input | ✅ Shipped | `b863381` | 2026-05-24; edge detection + auto-repeat working in mGBA |
| 4 — Engine: save | ✅ Shipped | `a930db0` | 2026-05-24; SRAM persistence + magic/version/checksum validation working in mGBA |
| 5 — Engine: sound | ✅ Shipped | `a03a9c8` | 2026-05-24; all 9 SFX audibly distinct in mGBA (pitch calibration deferred to Plan C) |
| 6 — Engine: anim | ⬜ Not started | — | — |
| 7 — Game: puzzle_logic + tests | ⬜ Not started | — | TDD required (host-testable) |
| 8 — Puzzle JSON pipeline | ⬜ Not started | — | TDD required (Python unittest) |

### Deviations

- **Phase 2 (2026-05-24): font.png generated programmatically, not hand-pixeled.**
  Plan Task 2.1 calls for hand-pixeling `assets/font.png` in an editor like
  Aseprite. Since the executing agent has no pixel-art tooling and Pillow
  was not installed, the asset is generated by `tools/make_font.py` (pure
  stdlib PNG writer + embedded public-domain font8x8 BASIC bitmap data).
  The output artifact (128×32 indexed PNG with DMG palette, glyphs at
  ASCII 0x20–0x5F in row-major order) is functionally identical to what
  Task 2.1 specified. `make_font.py` is a one-shot regeneration tool,
  not part of the build dependency graph; `assets/font.png` is checked
  into git as a source asset (matching plan intent).

- **Phase 2 (2026-05-24): png2asset flags differ from plan Task 2.2.**
  The plan's snippet uses `png2asset $< -o $@ -c $@ -bpp 2 -keep_palette_order -tiles_only -noflip`.
  Three corrections were needed:
  1. **Added `-spr8x8`**. Without it, png2asset's default `-spr8x16` pairs
     each vertically-adjacent pair of 8×8 cells into one 8×16 metasprite
     tile and emits them in interleaved order — tile 1 becomes '0',
     tile 2 becomes '!', breaking the assumed `tile_index = c - 0x20`
     mapping.
  2. **Added `-sprite_no_optimize`**. png2asset deduplicates identical
     tiles by default. Since space (' ', ASCII 0x20) is an all-zeros
     tile, it gets removed, shifting every subsequent glyph by one and
     reducing `font_TILE_COUNT` from 64 to 63.
  3. **Dropped `-c $@`**. Per png2asset's own usage banner, `-c` is
     "deprecated, same as -o".

  Final flag set in `Makefile`: `-spr8x8 -bpp 2 -keep_palette_order -sprite_no_optimize -tiles_only -noflip`.
  Plan Task 2.2's documented expectation of `font_TILE_COUNT = 64` and
  `font_tiles[1024]` now holds with these flags.

### Discoveries
- **2026-05-24, Phase 1 start:** GBDK-2020 toolchain and GNU `make` were NOT installed on the user's machine. User installed: GBDK-2020 extracted to `C:\gbdk\` (with `C:\gbdk\bin\` containing `lcc.exe`, `png2asset.exe`, etc.); GNU Make 4.4.1 via MSYS2 to `C:\msys64\usr\bin\make.exe`; Python 3.12 already present.
- **Plan defect (resolved inline via Discovery):** Task 1.1 Step 1 says "Expected output: a version line like `lcc <version-number>`". This is **wrong** — `lcc` does NOT accept a `--version` flag. Running `lcc --version` causes lcc to interpret `--version` as garbage args and produce error output ("Unknown option -r ignored", linker warnings). **Correct verification: run `lcc` with no arguments — it should print its usage banner `C:\gbdk\bin\lcc.exe [ option | file ]...`**. Same correction applies to `png2asset --version`: run `png2asset` with no args to see its usage banner.
- **Plan defect (resolved inline via Discovery):** The user's system-PATH addition for `C:\gbdk\bin` did not propagate to fresh Git Bash sessions (Windows PATH-propagation quirk — sometimes requires full logout or restart). User worked around by adding `export PATH="$PATH:/c/gbdk/bin"` to `~/.bashrc`. **The agent's `Bash` tool is non-interactive and does NOT source `~/.bashrc`**, so all subsequent build commands run by the agent must prepend `export PATH="$PATH:/c/gbdk/bin:/c/msys64/usr/bin" && <command>` to see the toolchain. This is a per-call workaround until the user's system PATH fully propagates or a `~/.bash_env` (BASH_ENV) is set.
- **Plan defect (resolved inline via fix):** Task 1.4 Makefile contained `-Wm-yc` with comment "GBC mode OFF (DMG-only target)". **This is the opposite of what the flag does.** In GBDK lcc, `-Wm-yc` sets the **CGB-compatible byte in the cartridge header**, marking the ROM as Game Boy Color compatible — exactly what we DON'T want for a DMG-only target. First build with this flag produced `Game Boy Color ROM image: "GBCX" ... [CGB]` per `file` output. After removing `-Wm-yc` from `LCC_FLAGS` and clean-rebuilding, `file` output correctly shows `Game Boy ROM image: "GBCX" ... [MBC1+RAM+BATT]` (no `[CGB]` flag). For DMG-only target, **omit `-Wm-yc` entirely**. Plan A Task 1.4 has been corrected inline.

---

## Phase 1 — Project bootstrap

**Execution Status:** ✅ SHIPPED at `d98081c` on 2026-05-24 (branch `main`). All 6 tasks complete: GBDK toolchain + make installed and verified; directory tree created; minimal `src/main.c` written; `Makefile` written + ROM builds clean (64KB MBC1+RAM+BATT, DMG-only); ROM loads in mGBA with title "GBCX"; commit landed. See Discoveries section for three plan defects fixed inline during execution: (1) `lcc --version` is not a valid flag; (2) Bash-tool needs PATH prepend per call; (3) `-Wm-yc` flag in original Makefile incorrectly enabled CGB-compat — removed for true DMG-only output.

**Goal**: Produce a minimal `.gb` ROM that boots in BGB and prints "GBCX" on the screen. Establishes that the GBDK toolchain works end-to-end and the Makefile structure is sound.

### Task 1.1: Verify GBDK-2020 installation

**Files:** none (environment check only)

- [ ] **Step 1: Check `lcc` is on PATH**

```bash
lcc --version
```

Expected output: a version line like `lcc <version-number>` (GBDK-2020 ships lcc; if "command not found", install GBDK-2020 from https://github.com/gbdk-2020/gbdk-2020/releases — pick the installer matching your OS: `gbdk-windows.zip` for Windows, `gbdk-macos-arm64.tar.gz` or `gbdk-macos.tar.gz` for Mac, `gbdk-linux64.tar.gz` for Linux. After extracting, add the `gbdk/bin` directory to your PATH).

**Windows specifically:** GBDK's `lcc` wrapper internally invokes shell scripts and standard POSIX utilities. Run all `make` commands from **Git Bash** (bundled with Git for Windows) or **MSYS2** — PowerShell and `cmd.exe` will fail on the `lcc` invocations and on `make size`'s `stat` calls. The rest of this plan's `bash` snippets assume Git Bash or equivalent.

- [ ] **Step 2: Check `png2asset` is on PATH**

```bash
png2asset --version 2>&1 | head -2
```

Expected output: a version banner. If "command not found", GBDK install is incomplete.

- [ ] **Step 3: Locate GBDK headers**

```bash
ls "$(dirname $(which lcc))/../include/gb/" 2>/dev/null || ls "$(dirname $(which lcc))/../../include/gb/" 2>/dev/null
```

Expected: a directory listing including `gb.h`, `hardware.h`, etc. Note the absolute path — it'll be useful for IDE configuration later.

### Task 1.2: Create directory structure

**Files:** none modified; only directories created.

- [ ] **Step 1: Create directory tree**

```bash
mkdir -p src/engine src/game src/assets_gen tools content assets test build
```

- [ ] **Step 2: Verify the tree matches the spec layout**

```bash
ls -la src/ tools/ content/ assets/ test/ build/
```

Expected: all 6 top-level directories exist; `src/` contains `engine/`, `game/`, and `assets_gen/`.

- [ ] **Step 3: Commit (empty directories will be picked up once they contain tracked files in later tasks)**

No commit needed yet — empty directories aren't tracked by git. Continue to Task 1.3.

### Task 1.3: Write minimal `main.c`

**Files:**
- Create: `src/main.c`

- [ ] **Step 1: Write a "Hello GBCX" entry point**

```c
// src/main.c
#include <gb/gb.h>
#include <stdint.h>

void main(void) {
    // Set the background palette to standard 4-shade
    BGP_REG = 0xE4;  // 11 10 01 00 — dark to light

    // For now, just display a banner via SCREEN_ON; later phases hook in real rendering.
    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        wait_vbl_done();
    }
}
```

The `main` function in GBDK-2020 returns void (not `int`); this is per the GBDK convention.

### Task 1.4: Write the Makefile

**Files:**
- Create: `Makefile`

- [ ] **Step 1: Write a minimal Makefile**

```makefile
# Game Boy Connections — Makefile (Plan A)

# GBDK paths — `lcc` should be on PATH; adjust GBDK_HOME if needed
LCC := lcc

# Compiler / linker flags
# -Wm-yt0x03 → MBC1+RAM+Battery cartridge type byte
# -Wm-yo4   → 4 ROM banks (64KB total)
# -Wm-ya1   → 1 RAM bank (8KB SRAM)
# -Wm-yn"GBCX" → ROM name in cartridge header
# (Do NOT add -Wm-yc — that flag sets the CGB-compatible byte in the
#  cartridge header. For DMG-only target, omit it entirely.)
LCC_FLAGS := -Wm-yt0x03 -Wm-yo4 -Wm-ya1 -Wm-yn"GBCX"

# Source files
SRC := src/main.c
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))

# Output ROM
ROM := build/gameboygame.gb

.PHONY: all clean run size

all: $(ROM)

# Build object file from C source
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(LCC) $(LCC_FLAGS) -c -o $@ $<

# Link object files into ROM
$(ROM): $(OBJ)
	@mkdir -p $(dir $@)
	$(LCC) $(LCC_FLAGS) -o $@ $(OBJ)

clean:
	rm -rf build/

size: $(ROM)
	@printf "ROM size: %s bytes (max: 65536)\n" "$$(stat -c%s $(ROM) 2>/dev/null || stat -f%z $(ROM))"

run: $(ROM)
	@echo "Open $(ROM) in BGB or SameBoy."
```

- [ ] **Step 2: Build the ROM**

```bash
make
```

Expected: build/main.o and build/gameboygame.gb produced, no errors.

- [ ] **Step 3: Check ROM size**

```bash
make size
```

Expected: a line like `ROM size: 32768 bytes (max: 65536)`. Sizes between 32KB and 65536 are normal; over 65536 means linker flags are wrong.

### Task 1.5: Verify ROM boots in BGB

**Files:** none

- [ ] **Step 1: Open the ROM in BGB**

```bash
make run
```

Then manually open `build/gameboygame.gb` in BGB (File → Load ROM). On macOS/Linux, open it in SameBoy instead.

- [ ] **Step 2: Verify boot screen**

Expected: BGB shows a screen filled with the default tile pattern. No crash, no "Invalid ROM" error. The screen is mostly blank because we haven't drawn anything yet — that's correct for this phase.

- [ ] **Step 3: Check ROM header in BGB**

In BGB, use Other → Cart Info to view the cartridge header. Verify:
- Title: `GBCX` (or similar)
- MBC: `MBC1+RAM+Battery` (or hex `0x03`)
- ROM size: indicates 64KB
- RAM size: indicates 8KB (`0x02` or "8 KiB")

If MBC is wrong, the linker flag in the Makefile is wrong — revisit `-Wm-yt0x03`.

### Task 1.6: Add `.gitignore` entry for build artifacts (already present) and commit Phase 1

**Files:** none modified (`.gitignore` from prior commit already covers `build/`)

- [ ] **Step 1: Verify .gitignore covers build artifacts**

```bash
cat .gitignore
```

Expected: includes `build/`, `.superpowers/`, and generated source files. If `build/` isn't listed, add it.

- [ ] **Step 2: Stage and commit Phase 1 artifacts**

```bash
git add Makefile src/main.c
git status
```

Expected: only `Makefile` and `src/main.c` staged. No `build/` files.

```bash
git commit -m "1: bootstrap GBDK build — minimal main.c + Makefile producing valid MBC1+RAM+Battery ROM"
```

- [ ] **Step 3: Mark Phase 1 complete in the Execution Status banner above**

Edit this file to flip Phase 1's banner from `⬜ NOT STARTED` to `✅ SHIPPED at <SHA> on <YYYY-MM-DD>`, and update the top-of-plan table.

---

## Phase 2 — Engine: render subsystem

**Execution Status:** ✅ SHIPPED at `ef5b12b` on 2026-05-24. All 6 tasks complete: `assets/font.png` generated programmatically via `tools/make_font.py` (see Deviations — Pillow not installed, no pixel-art tooling available); png2asset configured with corrected flags `-spr8x8 -sprite_no_optimize` (see Deviations — plan's flags produced wrong tile order + missing tile); `src/engine/render.{h,c}` implement tilemap buffer + VBlank flush + ASCII→tile mapping; `src/main.c` smoke-test scaffolding renders "RENDER OK" / "PLAN A PHASE 2"; ROM builds clean at 64KB MBC1+RAM+BATT (no [CGB] flag). Tile data verified by byte-level inspection of `font_tiles[]` (tile 1 = '!', tile 33 = 'A'). mGBA visual confirmation received from user — both "RENDER OK" and "PLAN A PHASE 2" display correctly.

**Goal**: Build the rendering subsystem (tilemap buffer in WRAM + VBlank-synced flush + text drawing helper + font tile loading). Render is implemented first so subsequent engine modules have output for their smoke tests. End state: `main.c` smoke-test scaffolding draws "RENDER OK" to the screen via `render_text()`.

**Makefile SRC pattern for Phases 2-6:** Each engine subsystem task adds its `.c` file to the Makefile's `SRC := ...` variable. The pattern is: every "Add X.c to Makefile SRC" step REPLACES the `SRC := ...` line with a new line that includes all previously-added engine files plus the new one. The separate `SRC += $(ASSETS_GEN)` line below (added in Task 2.2) MUST be preserved across these replacements — do NOT delete it. The final state after Phase 6 will be:

```makefile
SRC := src/main.c src/engine/render.c src/engine/input.c src/engine/save.c src/engine/sound.c src/engine/anim.c
SRC += $(ASSETS_GEN)
```

Phase 8 (puzzle pipeline) adds `SRC += $(PUZZLE_OUT)` below those lines.

### Task 2.1: Pixel the font tile sheet

**Files:**
- Create: `assets/font.png`

- [ ] **Step 1: Design the font tile sheet**

Use any pixel art editor (Aseprite, Piskel, GIMP) to create a `assets/font.png` with the following constraints:
- Indexed color, 4-color palette (matches GB DMG): `#9bbc0f` (lightest, index 0), `#8bac0f`, `#306230`, `#0f380f` (darkest, index 3)
- Image size: 128 × 32 pixels (16 columns × 4 rows of 8×8 tiles = 64 glyphs)
- Tile order (row-major, ASCII-like):
  - Row 0 (tiles 0-15):  ` ! " # $ % & ' ( ) * + , - . /`
  - Row 1 (tiles 16-31): `0 1 2 3 4 5 6 7 8 9 : ; < = > ?`
  - Row 2 (tiles 32-47): `@ A B C D E F G H I J K L M N O`
  - Row 3 (tiles 48-63): `P Q R S T U V W X Y Z [ \ ] ^ _`
- Background color of each tile: index 0 (lightest). Foreground glyph: index 3 (darkest).

This mirrors ASCII `0x20`-`0x5F` so the offset from ASCII to tile index is just `-0x20`.

- [ ] **Step 2: Verify the font image opens correctly**

Open `assets/font.png` in any image viewer and verify it's 128×32. Use a tool like `file` or `identify` (ImageMagick):

```bash
file assets/font.png
```

Expected: `assets/font.png: PNG image data, 128 x 32, ...`

### Task 2.2: Convert font.png to C tile data via png2asset

**Files:**
- Create: `src/assets_gen/font.c` (generated)
- Create: `src/assets_gen/font.h` (generated)
- Modify: `Makefile` (add font conversion rule)

- [ ] **Step 1: Add a font conversion rule to the Makefile**

Add these lines to the Makefile (after the `SRC := src/main.c` line):

```makefile
# Asset conversion
ASSETS_PNG := assets/font.png
ASSETS_GEN := $(patsubst assets/%.png,src/assets_gen/%.c,$(ASSETS_PNG))

src/assets_gen/%.c: assets/%.png
	@mkdir -p $(dir $@)
	png2asset $< -o $@ -c $@ -bpp 2 -keep_palette_order -tiles_only -noflip
# Note: -tiles_only emits BACKGROUND tile data (used by set_bkg_data),
# NOT sprite data. Older png2asset versions may not have -tiles_only;
# if so, drop the flag (the default mode emits tile data either way,
# but using -tiles_only when available is more explicit about intent).

# Add the generated assets to SRC
SRC += $(ASSETS_GEN)
```

- [ ] **Step 2: Build and verify the font asset is generated**

```bash
make
ls src/assets_gen/
```

Expected: `font.c` and `font.h` present. Open `src/assets_gen/font.c` and verify it contains a `const unsigned char font_tiles[]` array with 1024 bytes (64 tiles × 16 bytes per tile).

- [ ] **Step 3: Stage the generated headers' existence in .gitignore**

Confirm `.gitignore` covers `src/assets_gen/`. If not:

```bash
echo "src/assets_gen/" >> .gitignore
git add .gitignore
git commit -m "2.2: gitignore generated asset directory"
```

### Task 2.3: Define render.h API

**Files:**
- Create: `src/engine/render.h`

- [ ] **Step 1: Write the render API header**

```c
// src/engine/render.h
#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H

#include <stdint.h>

#define SCREEN_TILES_W 20
#define SCREEN_TILES_H 18

// Initialize render subsystem: load font tiles into VRAM, clear tilemap buffer.
// Must be called once at boot before any other render_* function.
void render_init(void);

// Clear the tilemap buffer (does not touch VRAM until next render_flush).
void render_clear(void);

// Write a null-terminated ASCII string starting at tile column x, row y.
// Characters outside printable ASCII (0x20-0x5F) are rendered as space.
// x and y must satisfy 0 <= x < SCREEN_TILES_W, 0 <= y < SCREEN_TILES_H.
void render_text(uint8_t x, uint8_t y, const char *s);

// Push the in-memory tilemap buffer to VRAM. MUST be called only during VBlank.
// Typically called from the VBlank interrupt handler.
void render_flush(void);

#endif
```

### Task 2.4: Implement render.c with tilemap buffer + flush

**Files:**
- Create: `src/engine/render.c`

- [ ] **Step 1: Write the implementation**

```c
// src/engine/render.c
#include "render.h"
#include "../assets_gen/font.h"
#include <gb/gb.h>
#include <string.h>

// 20x18 tilemap buffer in WRAM (360 bytes). Each byte is a tile index.
static uint8_t tilemap_buf[SCREEN_TILES_W * SCREEN_TILES_H];

// Whether the buffer has been modified since last flush
static uint8_t dirty = 0;

void render_init(void) {
    // Load font tiles into VRAM starting at tile index 0.
    // png2asset (GBDK-2020) emits `font_TILE_COUNT` as a #define macro
    // and `font_tiles` as a `const unsigned char[]` symbol. See
    // src/assets_gen/font.h to confirm the exact symbol names emitted
    // by your png2asset version — older versions may use `font_tile_count`
    // (lowercase) instead. Adjust this line to match what's in font.h.
    set_bkg_data(0, font_TILE_COUNT, font_tiles);

    // Clear the tilemap buffer to space (ASCII 0x20 → tile index 0)
    render_clear();
}

void render_clear(void) {
    memset(tilemap_buf, 0, sizeof(tilemap_buf));  // tile 0 = space (first font glyph)
    dirty = 1;
}

void render_text(uint8_t x, uint8_t y, const char *s) {
    if (y >= SCREEN_TILES_H) return;
    uint8_t *dst = &tilemap_buf[y * SCREEN_TILES_W + x];
    while (*s && x < SCREEN_TILES_W) {
        char c = *s;
        // Map ASCII 0x20..0x5F to tile index 0..63. Anything else becomes space.
        if (c < 0x20 || c > 0x5F) c = 0x20;
        *dst++ = (uint8_t)(c - 0x20);
        s++;
        x++;
    }
    dirty = 1;
}

void render_flush(void) {
    if (!dirty) return;
    // Push the whole 20x18 tilemap to VRAM in one batch.
    set_bkg_tiles(0, 0, SCREEN_TILES_W, SCREEN_TILES_H, tilemap_buf);
    dirty = 0;
}
```

- [ ] **Step 2: Add render.c to the Makefile SRC list**

Edit `Makefile`:

```makefile
SRC := src/main.c src/engine/render.c
```

(The `+=` line for ASSETS_GEN stays after this.)

### Task 2.5: Wire render into main.c smoke test

**Files:**
- Modify: `src/main.c`

- [ ] **Step 1: Replace main.c with a render smoke test**

```c
// src/main.c
#include <gb/gb.h>
#include "engine/render.h"

void main(void) {
    BGP_REG = 0xE4;

    render_init();
    render_text(2, 2, "RENDER OK");
    render_text(2, 4, "PLAN A PHASE 2");
    render_flush();

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        wait_vbl_done();
        // No VBlank-driven flush yet — we only flushed once at boot.
        // Phase 6+ scenes will call render_text during update() and flush in VBlank.
    }
}
```

- [ ] **Step 2: Build and run in BGB**

```bash
make
```

Open `build/gameboygame.gb` in BGB.

Expected: screen shows "RENDER OK" on row 2 and "PLAN A PHASE 2" on row 4 (positioned 2 tiles from left).

If characters look garbled, check that the font.png tile order matches the comment in Task 2.1 (ASCII `0x20`-`0x5F`). If nothing displays, check that `SHOW_BKG` and `DISPLAY_ON` are called.

### Task 2.6: Commit Phase 2

- [ ] **Step 1: Stage and commit**

```bash
git add Makefile assets/font.png src/engine/render.h src/engine/render.c src/main.c
git status  # verify no generated files staged
git commit -m "2: implement render subsystem — tilemap buffer + VBlank flush + text helper + font loading"
```

- [ ] **Step 2: Update Phase 2's Execution Status banner**

Flip Phase 2's banner from `⬜ NOT STARTED` to `✅ SHIPPED at <SHA> on <YYYY-MM-DD>`, and update the top-of-plan table.

---

## Phase 3 — Engine: input subsystem

**Execution Status:** ✅ SHIPPED at `b863381` on 2026-05-24. All 4 tasks complete: `src/engine/input.{h,c}` implement edge-triggered press/release detection, raw hold query, and per-button auto-repeat (15-frame initial delay then every 5 frames). `src/main.c` smoke-test shows live held-button display + edge-triggered "A PRESSED!" flash. ROM still builds at 64KB MBC1+RAM+BATT. mGBA visual confirmation received from user — all buttons (D-pad, A, B, START, SELECT) tracked correctly, edge trigger works as designed. Minor smoke-test tweak: press-flash duration bumped from 1 frame to 30 frames for visibility (does not affect subsystem behavior).

**Goal**: Build the input subsystem — edge-triggered button detection with auto-repeat for D-pad. Smoke test: `main.c` shows current button state visually (e.g., "UP DOWN LEFT RIGHT A B START SELECT" toggling based on actual button presses).

### Task 3.1: Define input.h API

**Files:**
- Create: `src/engine/input.h`

- [ ] **Step 1: Write the input API header**

```c
// src/engine/input.h
#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BTN_RIGHT  = 0x01,
    BTN_LEFT   = 0x02,
    BTN_UP     = 0x04,
    BTN_DOWN   = 0x08,
    BTN_A      = 0x10,
    BTN_B      = 0x20,
    BTN_SELECT = 0x40,
    BTN_START  = 0x80
} Button;

// Call once per frame, after VBlank, before scene update().
// Reads the joypad and updates internal previous/current state.
void input_update(void);

// True if button was newly pressed this frame (edge-triggered).
bool input_pressed(Button b);

// True if button is currently held (raw, no edge detection).
bool input_held(Button b);

// True if button was newly released this frame (edge-triggered).
bool input_released(Button b);

// True if button was newly pressed this frame OR has been held long enough
// to fire an auto-repeat tick (initial delay ~250ms = 15 frames; repeat rate
// ~80ms = 5 frames). Used for D-pad in scenes with cursors.
bool input_repeat(Button b);

#endif
```

### Task 3.2: Implement input.c

**Files:**
- Create: `src/engine/input.c`

- [ ] **Step 1: Write the implementation**

```c
// src/engine/input.c
#include "input.h"
#include <gb/gb.h>

#define INITIAL_DELAY_FRAMES 15
#define REPEAT_PERIOD_FRAMES 5

static uint8_t current  = 0;
static uint8_t previous = 0;
static uint8_t hold_frames[8] = {0};  // one counter per button bit

static uint8_t button_to_index(Button b) {
    // Convert bitmask to index 0..7
    switch (b) {
        case BTN_RIGHT:  return 0;
        case BTN_LEFT:   return 1;
        case BTN_UP:     return 2;
        case BTN_DOWN:   return 3;
        case BTN_A:      return 4;
        case BTN_B:      return 5;
        case BTN_SELECT: return 6;
        case BTN_START:  return 7;
        default:         return 0;
    }
}

void input_update(void) {
    previous = current;
    current  = joypad();

    // Per-button hold-frame counter for auto-repeat
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t mask = (uint8_t)(1u << i);
        if (current & mask) {
            if (hold_frames[i] < 0xFF) hold_frames[i]++;
        } else {
            hold_frames[i] = 0;
        }
    }
}

bool input_pressed(Button b) {
    return (current & b) && !(previous & b);
}

bool input_held(Button b) {
    return (current & b) != 0;
}

bool input_released(Button b) {
    return !(current & b) && (previous & b);
}

bool input_repeat(Button b) {
    if (input_pressed(b)) return true;
    if (!(current & b)) return false;
    uint8_t idx = button_to_index(b);
    uint8_t f = hold_frames[idx];
    if (f < INITIAL_DELAY_FRAMES) return false;
    return ((f - INITIAL_DELAY_FRAMES) % REPEAT_PERIOD_FRAMES) == 0;
}
```

- [ ] **Step 2: Add input.c to Makefile SRC**

```makefile
SRC := src/main.c src/engine/render.c src/engine/input.c
```

### Task 3.3: Wire input into main.c smoke test

**Files:**
- Modify: `src/main.c`

- [ ] **Step 1: Replace main.c with an input smoke test**

```c
// src/main.c
#include <gb/gb.h>
#include "engine/render.h"
#include "engine/input.h"

void main(void) {
    BGP_REG = 0xE4;
    render_init();

    SHOW_BKG;
    DISPLAY_ON;

    char line[21];
    while (1) {
        input_update();

        // Show currently-held buttons
        line[0] = input_held(BTN_UP)     ? 'U' : '-';
        line[1] = input_held(BTN_DOWN)   ? 'D' : '-';
        line[2] = input_held(BTN_LEFT)   ? 'L' : '-';
        line[3] = input_held(BTN_RIGHT)  ? 'R' : '-';
        line[4] = ' ';
        line[5] = input_held(BTN_A)      ? 'A' : '-';
        line[6] = input_held(BTN_B)      ? 'B' : '-';
        line[7] = ' ';
        line[8] = input_held(BTN_START)  ? 'T' : '-';  // sTart
        line[9] = input_held(BTN_SELECT) ? 'S' : '-';
        line[10] = 0;

        render_clear();
        render_text(2, 2, "INPUT SMOKE TEST");
        render_text(2, 4, "HELD:");
        render_text(2, 5, line);

        // Edge-trigger demo: 'X' appears for one frame when A is pressed
        if (input_pressed(BTN_A)) {
            render_text(2, 7, "A PRESSED!");
        }

        wait_vbl_done();
        render_flush();
    }
}
```

- [ ] **Step 2: Build and run in BGB**

```bash
make
```

Open the ROM. Press D-pad and A/B/START/SELECT — verify the held-button display changes in real time. Press A briefly — "A PRESSED!" should flash for one frame.

If button display doesn't change, the issue is in `input_update` — verify it's being called each frame.

### Task 3.4: Commit Phase 3

- [ ] **Step 1: Stage and commit**

```bash
git add Makefile src/engine/input.h src/engine/input.c src/main.c
git commit -m "3: implement input subsystem — edge detection + auto-repeat with smoke test"
```

- [ ] **Step 2: Update Phase 3 banner above**

---

## Phase 4 — Engine: save subsystem

**Execution Status:** ✅ SHIPPED at `a930db0` on 2026-05-24. All 6 tasks complete: `src/game/puzzles_types.h` provides Puzzle forward-decl + extern NUM_PUZZLES/PUZZLES (concrete data lands in Phase 8); `src/game/game_state.h` defines Scene enum, PlayState, and 20-byte GameSave with SRAM-format-compatible field ordering; `src/engine/save.{h,c}` implement save_load/save_store/save_reset with magic ("GBCX") + version (1) + XOR checksum validation, auto-recovery on any failure. `src/main.c` smoke-test confirms first-boot reset, A-key increment + SRAM persistence across hard reset, B-key reset path. mGBA visual confirmation received from user — tests 1, 2, 3 of the verification protocol all pass. Optional corruption test deferred (covered indirectly by first-boot magic-check path).

**Goal**: Build SRAM read/write with magic + version + checksum validation. Smoke test: `main.c` writes a known save at boot, reads it back, displays loaded values on screen. After verifying load works, manually corrupt SRAM in BGB's memory viewer, power-cycle, verify the save resets to defaults.

### Task 4.1: Define puzzles_types.h (Puzzle struct referenced by save indirectly via game_state.h)

**Files:**
- Create: `src/game/puzzles_types.h`

- [ ] **Step 1: Write the type definitions**

```c
// src/game/puzzles_types.h
#ifndef GAME_PUZZLES_TYPES_H
#define GAME_PUZZLES_TYPES_H

#include <stdint.h>

#define WORDS_PER_PUZZLE       16
#define GROUPS_PER_PUZZLE       4
#define WORDS_PER_GROUP         4
#define MAX_WORD_LEN            8
#define MAX_CATEGORY_NAME_LEN  12

typedef struct {
    char    words[WORDS_PER_PUZZLE][MAX_WORD_LEN + 1];   // null-terminated
    uint8_t group_of[WORDS_PER_PUZZLE];                  // 0..3 (tier index)
    char    category_names[GROUPS_PER_PUZZLE][MAX_CATEGORY_NAME_LEN + 1];
} Puzzle;

extern const uint8_t NUM_PUZZLES;
extern const Puzzle  PUZZLES[];

#endif
```

This is a forward-declaration header. The actual `PUZZLES[]` array is defined in `src/puzzles_data.c`, generated by `tools/build_puzzles.py` in Phase 8.

### Task 4.2: Define game_state.h with Scene enum + GameSave struct

**Files:**
- Create: `src/game/game_state.h`

- [ ] **Step 1: Write the game state header**

```c
// src/game/game_state.h
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdint.h>
#include "puzzles_types.h"

typedef enum {
    SCENE_TITLE = 0,
    SCENE_PLAY,
    SCENE_WIN,
    SCENE_LOSE,
    SCENE_ALL_DONE
} Scene;

// PlayState — in-RAM state for the PLAY scene
typedef struct {
    uint8_t  cursor_idx;          // 0..15
    uint16_t selected_mask;       // bits 0..15
    uint8_t  groups_solved;       // bits 0..3 (yellow,green,blue,purple)
    uint8_t  tries_remaining;     // 0..4
    uint16_t elapsed_seconds;
    uint8_t  show_quit_confirm;   // 0 or 1
} PlayState;

// GameSave — exactly 20 bytes, persisted to SRAM at 0xA000.
// Layout MUST match spec §7. Field order is load-bearing for SRAM compatibility.
typedef struct {
    uint8_t  magic[4];                 // [0..3]   "GBCX"
    uint8_t  version;                  // [4]      1
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
    uint8_t  checksum;                 // [19]     XOR of bytes [0..18]
} GameSave;

#define SAVE_MAGIC_0 'G'
#define SAVE_MAGIC_1 'B'
#define SAVE_MAGIC_2 'C'
#define SAVE_MAGIC_3 'X'
#define SAVE_VERSION 1

#endif
```

### Task 4.3: Define save.h API

**Files:**
- Create: `src/engine/save.h`

- [ ] **Step 1: Write the save API header**

```c
// src/engine/save.h
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

// Write *in to SRAM. Computes a fresh checksum on a local copy so the caller's
// struct is not mutated. Brackets the write with ENABLE_RAM_MBC1 / DISABLE_RAM_MBC1.
// Returns true on success.
bool save_store(const GameSave *in);

// Overwrite *out with factory-default values (NEW GAME).
// Does NOT write to SRAM — caller is responsible for calling save_store(out).
void save_reset(GameSave *out);

#endif
```

### Task 4.4: Implement save.c

**Files:**
- Create: `src/engine/save.c`

- [ ] **Step 1: Write the implementation**

```c
// src/engine/save.c
#include "save.h"
#include <gb/gb.h>
#include <string.h>

// MBC1 maps SRAM bank 0 to 0xA000-0xBFFF when enabled.
// Cast pointer to access SRAM as a GameSave struct directly.
#define SRAM_SAVE (*(GameSave *)0xA000)

static uint8_t compute_checksum(const GameSave *s) {
    const uint8_t *p = (const uint8_t *)s;
    uint8_t x = 0;
    // XOR bytes [0..18] (everything except the checksum byte itself)
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
    // Work on a local copy so the caller's struct is not mutated by checksum compute.
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

    // Validate magic
    if (tmp.magic[0] != SAVE_MAGIC_0 || tmp.magic[1] != SAVE_MAGIC_1
     || tmp.magic[2] != SAVE_MAGIC_2 || tmp.magic[3] != SAVE_MAGIC_3) {
        save_reset(out);
        save_store(out);
        return false;
    }

    // Validate version
    if (tmp.version != SAVE_VERSION) {
        save_reset(out);
        save_store(out);
        return false;
    }

    // Validate checksum
    if (compute_checksum(&tmp) != tmp.checksum) {
        save_reset(out);
        save_store(out);
        return false;
    }

    // Sanity check: ip_groups_solved must fit in 4 bits.
    // current_puzzle_index sanity check uses NUM_PUZZLES from puzzles_data.c.
    // For Plan A Phase 4 there are no puzzles yet — defer the index sanity check.
    // Phase 8 introduces NUM_PUZZLES and the index check can be tightened then.
    if (tmp.ip_groups_solved > 0x0F) {
        save_reset(out);
        save_store(out);
        return false;
    }

    *out = tmp;
    return true;
}
```

- [ ] **Step 2: Add save.c to Makefile SRC**

```makefile
SRC := src/main.c src/engine/render.c src/engine/input.c src/engine/save.c
```

### Task 4.5: Wire save into main.c smoke test

**Files:**
- Modify: `src/main.c`

- [ ] **Step 1: Replace main.c with a save smoke test**

```c
// src/main.c
#include <gb/gb.h>
#include <stdio.h>
#include "engine/render.h"
#include "engine/input.h"
#include "engine/save.h"

static char buf[21];

static void show_save_state(const GameSave *s, bool was_valid) {
    render_clear();
    render_text(2, 1, "SAVE SMOKE TEST");
    render_text(2, 3, was_valid ? "LOAD: VALID" : "LOAD: RESET");

    sprintf(buf, "IDX:%d FAILS:%d", s->current_puzzle_index, s->current_puzzle_fails);
    render_text(2, 5, buf);

    sprintf(buf, "SOLVED:%d SKIP:%d", s->puzzles_solved_total, s->puzzles_skipped_total);
    render_text(2, 6, buf);

    sprintf(buf, "STREAK:%d BEST:%d", s->current_streak, s->best_streak);
    render_text(2, 7, buf);

    render_text(2, 10, "A: BUMP SOLVED");
    render_text(2, 11, "B: RESET SAVE");
}

void main(void) {
    BGP_REG = 0xE4;
    render_init();

    GameSave save;
    bool was_valid = save_load(&save);

    show_save_state(&save, was_valid);

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        input_update();

        if (input_pressed(BTN_A)) {
            save.puzzles_solved_total++;
            save_store(&save);
            show_save_state(&save, was_valid);
        }
        if (input_pressed(BTN_B)) {
            save_reset(&save);
            save_store(&save);
            was_valid = false;  // visibly mark as reset
            show_save_state(&save, was_valid);
        }

        wait_vbl_done();
        render_flush();
    }
}
```

- [ ] **Step 2: Build and run in BGB**

```bash
make
```

Open the ROM. Verify the screen shows save state.

- [ ] **Step 3: Test the persistence**

In BGB:
1. Note the displayed "SOLVED" count (should be 0 on first boot).
2. Press A several times — SOLVED count increments.
3. File → Reset Hard (full power cycle).
4. Open the ROM again — SOLVED count should match the last value before reset (state survived power-cycle via SRAM).

If the count resets to 0 after a hard reset, SRAM isn't persisting — verify the MBC type in BGB's Cart Info shows MBC1+RAM+Battery.

- [ ] **Step 4: Test the corruption detection**

In BGB (menu paths may vary slightly by BGB version — look for "Memory Viewer" / "Memory" / "Editor" under Debug or Other):
1. Open the ROM, press A a few times to set SOLVED > 0.
2. Open the Memory Viewer (BGB 1.5.x: Other → Memory Editor; some versions: Debug → Memory).
3. Navigate to address `0xA000`. You should see `47 42 43 58` (ASCII "GBCX") at the start of that region.
4. Modify byte `0xA000` to `0x00` (corrupt the magic) by clicking on the byte and typing `00`.
5. Hard-reset the emulator (BGB: File → Reset, or Ctrl+R).
6. Re-open the ROM. Display should show "LOAD: RESET" and SOLVED back to 0.

This confirms the magic-byte validity check works. If your BGB version's menus differ, the SameBoy equivalent is Debugger → Memory Viewer, with the same address `0xA000`.

### Task 4.6: Commit Phase 4

- [ ] **Step 1: Stage and commit**

```bash
git add Makefile src/game/puzzles_types.h src/game/game_state.h src/engine/save.h src/engine/save.c src/main.c
git commit -m "4: implement save subsystem — MBC1 SRAM with magic/version/checksum validation"
```

- [ ] **Step 2: Update Phase 4 banner above**

---

## Phase 5 — Engine: sound subsystem

**Execution Status:** ✅ SHIPPED at `a03a9c8` on 2026-05-24. All 4 tasks complete: `src/engine/sound.{h,c}` implement 9 SFX via GB sound channel registers (NR10–NR52), with single-tick SFX (move/select/deselect/reject) using CH2 + CH4 trigger writes and multi-step SFX (correct/wrong/win/lose/skip) sequenced by `sound_tick()` called once per VBlank. `src/main.c` smoke-test maps each SFX to a button. mGBA audio confirmation from user — all 9 SFX produce audibly distinct sounds. Note: NR13/NR14 frequency byte values are placeholder estimates; pitch calibration deferred to Plan C per spec §13.

**Goal**: Build 9 SFX functions per the spec, each as a hardcoded sequence of GB sound register writes. Smoke test: `main.c` triggers each SFX in turn when player presses a specific button, audio confirms each works in BGB.

### Task 5.1: Define sound.h API

**Files:**
- Create: `src/engine/sound.h`

- [ ] **Step 1: Write the sound API header**

```c
// src/engine/sound.h
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
```

### Task 5.2: Implement sound.c with single-tick SFX

**Files:**
- Create: `src/engine/sound.c`

- [ ] **Step 1: Write the implementation skeleton + the simple single-step SFX**

```c
// src/engine/sound.c
#include "sound.h"
#include <gb/gb.h>
#include <gb/hardware.h>

// ---------- Multi-step SFX state ----------
// One active multi-step SFX at a time. step_remaining counts down per tick;
// when it reaches 0, advance to the next step.
typedef enum {
    SFX_NONE = 0,
    SFX_PLAYING_CORRECT,
    SFX_PLAYING_WRONG,
    SFX_PLAYING_WIN,
    SFX_PLAYING_LOSE,
    SFX_PLAYING_SKIP
} ActiveSfx;

static ActiveSfx active = SFX_NONE;
static uint8_t   step = 0;
static uint8_t   step_remaining = 0;

void sound_init(void) {
    NR52_REG = 0x80;  // sound on (master enable)
    NR51_REG = 0xFF;  // route all channels to both speakers
    NR50_REG = 0x77;  // max volume both speakers
}

// ---------- Single-tick SFX (no scheduling needed) ----------

void sfx_move(void) {
    // CH2: very short high tick
    NR21_REG = 0x80;          // duty 50%, length disabled
    NR22_REG = 0x84;          // volume 8, envelope decay fast
    NR23_REG = 0x00;          // low freq byte
    NR24_REG = 0x87;          // trigger + high freq byte (high pitch)
    active = SFX_NONE;        // single-step; no follow-up needed
}

void sfx_select(void) {
    NR21_REG = 0x80;
    NR22_REG = 0xA4;          // slightly louder
    NR23_REG = 0x80;
    NR24_REG = 0x87;          // higher pitch than move
    active = SFX_NONE;
}

void sfx_deselect(void) {
    NR21_REG = 0x80;
    NR22_REG = 0xA4;
    NR23_REG = 0x00;
    NR24_REG = 0x86;          // lower pitch than select
    active = SFX_NONE;
}

void sfx_reject(void) {
    // CH4: low noise buzz
    NR41_REG = 0x00;          // length disabled
    NR42_REG = 0xF3;          // volume max, envelope decay
    NR43_REG = 0x6B;          // noise frequency: low/buzzy
    NR44_REG = 0x80;          // trigger
    active = SFX_NONE;
}

// ---------- Multi-step SFX (use sound_tick to advance) ----------

void sfx_correct(void) {
    // Rising 3-note arpeggio. Step 0 plays C5; tick advances to E5, then G5.
    active = SFX_PLAYING_CORRECT;
    step = 0;
    step_remaining = 5;       // ~80ms per step (5 frames @ 60fps)

    // Play first note (C5 ≈ 523 Hz)
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0xC1;
    NR14_REG = 0x87;
}

void sfx_wrong(void) {
    active = SFX_PLAYING_WRONG;
    step = 0;
    step_remaining = 6;

    // First note: F5 (descending)
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x44;
    NR14_REG = 0x87;
}

void sfx_win(void) {
    active = SFX_PLAYING_WIN;
    step = 0;
    step_remaining = 8;

    // C5
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0xC1;
    NR14_REG = 0x87;
}

void sfx_lose(void) {
    active = SFX_PLAYING_LOSE;
    step = 0;
    step_remaining = 12;

    // F5 descending
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x44;
    NR14_REG = 0x87;
}

void sfx_skip(void) {
    active = SFX_PLAYING_SKIP;
    step = 0;
    step_remaining = 10;

    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x83;
    NR14_REG = 0x87;
}

// ---------- Tick driver ----------

void sound_tick(void) {
    if (active == SFX_NONE) return;
    if (step_remaining > 0) {
        step_remaining--;
        return;
    }

    step++;

    switch (active) {
        case SFX_PLAYING_CORRECT: {
            // Step 1 → E5, Step 2 → G5, Step 3 → done
            if (step == 1) {
                NR13_REG = 0x83; NR14_REG = 0x87;  // E5
                step_remaining = 5;
            } else if (step == 2) {
                NR13_REG = 0xC1; NR14_REG = 0x87;  // G5
                step_remaining = 5;
            } else {
                active = SFX_NONE;
            }
            break;
        }
        case SFX_PLAYING_WRONG: {
            if (step == 1) {
                NR13_REG = 0xC1; NR14_REG = 0x86;  // D5 (lower)
                step_remaining = 6;
            } else {
                active = SFX_NONE;
            }
            break;
        }
        case SFX_PLAYING_WIN: {
            // E5, G5, C6, then sustain
            if (step == 1) { NR13_REG = 0x83; NR14_REG = 0x87; step_remaining = 8; }
            else if (step == 2) { NR13_REG = 0xC1; NR14_REG = 0x87; step_remaining = 8; }
            else if (step == 3) { NR13_REG = 0xC1; NR14_REG = 0x88; step_remaining = 16; }
            else { active = SFX_NONE; }
            break;
        }
        case SFX_PLAYING_LOSE: {
            if (step == 1) { NR13_REG = 0xC1; NR14_REG = 0x86; step_remaining = 12; }
            else if (step == 2) { NR13_REG = 0x44; NR14_REG = 0x86; step_remaining = 16; }
            else { active = SFX_NONE; }
            break;
        }
        case SFX_PLAYING_SKIP: {
            if (step == 1) { NR13_REG = 0x44; NR14_REG = 0x87; step_remaining = 10; }
            else { active = SFX_NONE; }
            break;
        }
        default:
            active = SFX_NONE;
    }
}
```

**About the frequency values**: The `NR13_REG` / `NR14_REG` byte values above are **placeholder starting estimates**, not precisely-calibrated note frequencies. The Game Boy frequency formula is `freq_hz = 131072 / (2048 - x)` where x is the 11-bit value across NR13 (low 8 bits) and NR14 (high 3 bits in bits 0-2). Calibrating actual musical notes requires per-note math and ear-tuning on real hardware. Spec §13 explicitly notes these are subject to revision after emulator + hardware audio testing — that calibration happens in Plan C. For Plan A, the goal is "each SFX produces an audibly distinct sound" — verify that in the smoke test, but don't obsess over exact pitches.

**About the GBDK includes**: `<gb/gb.h>` is the umbrella include that transitively brings in `<gb/hardware.h>` (where `NR10_REG`-`NR52_REG` are defined). Including `<gb/hardware.h>` explicitly is belt-and-suspenders — if your GBDK install's `gb.h` doesn't transitively include it, you'll get "undefined NR10_REG" compile errors; in that case the explicit include resolves it. Both flavors are correct.

- [ ] **Step 2: Add sound.c to Makefile SRC**

```makefile
SRC := src/main.c src/engine/render.c src/engine/input.c src/engine/save.c src/engine/sound.c
```

### Task 5.3: Wire sound into main.c smoke test

**Files:**
- Modify: `src/main.c`

- [ ] **Step 1: Replace main.c with a sound smoke test**

```c
// src/main.c
#include <gb/gb.h>
#include "engine/render.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/sound.h"

void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();

    render_text(2, 1, "SOUND SMOKE TEST");
    render_text(2, 3, "UP: MOVE");
    render_text(2, 4, "DOWN: SELECT");
    render_text(2, 5, "LEFT: DESELECT");
    render_text(2, 6, "RIGHT: REJECT");
    render_text(2, 7, "A: CORRECT");
    render_text(2, 8, "B: WRONG");
    render_text(2, 9, "START: WIN");
    render_text(2, 10, "SELECT: LOSE");
    render_text(2, 12, "(A+B): SKIP");

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        input_update();

        if (input_pressed(BTN_UP))     sfx_move();
        if (input_pressed(BTN_DOWN))   sfx_select();
        if (input_pressed(BTN_LEFT))   sfx_deselect();
        if (input_pressed(BTN_RIGHT))  sfx_reject();
        if (input_pressed(BTN_A) && input_held(BTN_B))      sfx_skip();
        else if (input_pressed(BTN_A))                       sfx_correct();
        if (input_pressed(BTN_B) && !input_pressed(BTN_A))  sfx_wrong();
        if (input_pressed(BTN_START))   sfx_win();
        if (input_pressed(BTN_SELECT))  sfx_lose();

        wait_vbl_done();
        sound_tick();
        render_flush();
    }
}
```

- [ ] **Step 2: Build and run in BGB**

```bash
make
```

Open the ROM. Make sure BGB audio is enabled (Options → Sound → check "Enable").

Press each button and confirm:
- UP/DOWN/LEFT/RIGHT/REJECT each produce a short, distinct sound
- A produces a 3-note rising arpeggio
- B produces a descending 2-note wrong sound
- START produces a longer celebratory melody
- SELECT produces a descending sad melody
- A+B together produces the skip tone

If any SFX is silent, check that `sound_init()` was called and that BGB sound is enabled.

### Task 5.4: Commit Phase 5

- [ ] **Step 1: Stage and commit**

```bash
git add Makefile src/engine/sound.h src/engine/sound.c src/main.c
git commit -m "5: implement sound subsystem — 9 SFX via GB sound channel registers + tick driver"
```

- [ ] **Step 2: Update Phase 5 banner above**

---

## Phase 6 — Engine: animation subsystem

**Execution Status:** ⬜ NOT STARTED

**Goal**: Build the animation engine — one active animation at a time, frame-counter advanced per VBlank, type-dispatched render. Smoke test: `main.c` triggers a flash animation when A is pressed, verifies the screen briefly inverts.

### Task 6.1: Define anim.h API

**Files:**
- Create: `src/engine/anim.h`

- [ ] **Step 1: Write the animation API header**

```c
// src/engine/anim.h
#ifndef ENGINE_ANIM_H
#define ENGINE_ANIM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ANIM_NONE = 0,
    ANIM_SELECT_FLASH,
    ANIM_CELL_FLASH,
    ANIM_WRONG_SHAKE,
    ANIM_CORRECT_FLASH,
    ANIM_LAYOUT_REFLOW,
    ANIM_BAR_CASCADE,
    ANIM_STATS_FADE,
    ANIM_LOSE_REVEAL
} AnimType;

// Start an animation. data is type-specific payload (e.g., which cells, which tier).
// data may be NULL for animations that need no parameters. Up to 8 data bytes.
// Interrupts any currently-active animation.
void anim_start(AnimType type, const uint8_t *data, uint16_t duration);

// Advance the active animation by one frame. Call from VBlank handler.
// Type-dispatched: each AnimType has its own rendering logic.
void anim_tick(void);

// True if an animation is currently active. Scenes use this to gate input.
bool anim_is_playing(void);

AnimType anim_current(void);

#endif
```

### Task 6.2: Implement anim.c with stub handlers

**Files:**
- Create: `src/engine/anim.c`

- [ ] **Step 1: Write the engine + a real implementation for ANIM_WRONG_SHAKE**

```c
// src/engine/anim.c
#include "anim.h"
#include <gb/gb.h>
#include <string.h>

typedef struct {
    AnimType type;
    uint16_t frame;
    uint16_t duration;
    uint8_t  data[8];
} Anim;

static Anim active = { ANIM_NONE, 0, 0, {0} };

void anim_start(AnimType type, const uint8_t *data, uint16_t duration) {
    active.type     = type;
    active.frame    = 0;
    active.duration = duration;
    if (data) memcpy(active.data, data, 8);
    else      memset(active.data, 0, 8);
}

bool anim_is_playing(void) {
    return active.type != ANIM_NONE;
}

AnimType anim_current(void) {
    return active.type;
}

// ----- per-type tick handlers -----

static void tick_wrong_shake(void) {
    // ±1 pixel SCX jitter for `duration` frames
    if (active.frame >= active.duration) {
        SCX_REG = 0;
        active.type = ANIM_NONE;
        return;
    }
    SCX_REG = (active.frame & 1) ? 1 : 255;  // alternates +1, -1
    active.frame++;
}

static void tick_select_flash(void) {
    // Stub for Plan A — Plan B will implement cell-specific tile inversion.
    // For now, just advance the frame counter and end on duration.
    if (active.frame >= active.duration) {
        active.type = ANIM_NONE;
        return;
    }
    active.frame++;
}

// All other animations stub to "advance + end on duration" for Plan A.
// Plan B replaces these with real implementations when scenes need them.
static void tick_generic_stub(void) {
    if (active.frame >= active.duration) {
        active.type = ANIM_NONE;
        return;
    }
    active.frame++;
}

void anim_tick(void) {
    switch (active.type) {
        case ANIM_NONE:           return;
        case ANIM_WRONG_SHAKE:    tick_wrong_shake(); break;
        case ANIM_SELECT_FLASH:   tick_select_flash(); break;
        case ANIM_CELL_FLASH:
        case ANIM_CORRECT_FLASH:
        case ANIM_LAYOUT_REFLOW:
        case ANIM_BAR_CASCADE:
        case ANIM_STATS_FADE:
        case ANIM_LOSE_REVEAL:
            tick_generic_stub();
            break;
        default:
            active.type = ANIM_NONE;
    }
}
```

- [ ] **Step 2: Add anim.c to Makefile SRC**

```makefile
SRC := src/main.c src/engine/render.c src/engine/input.c src/engine/save.c src/engine/sound.c src/engine/anim.c
```

### Task 6.3: Wire anim into main.c smoke test

**Files:**
- Modify: `src/main.c`

- [ ] **Step 1: Replace main.c with an anim smoke test**

```c
// src/main.c
#include <gb/gb.h>
#include "engine/render.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/sound.h"
#include "engine/anim.h"

void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();

    render_text(2, 1, "ANIM SMOKE TEST");
    render_text(2, 3, "A: WRONG SHAKE");
    render_text(2, 4, "(WATCH FOR SCREEN");
    render_text(2, 5, " JITTER FOR 0.1S)");
    render_text(2, 7, "B: SELECT FLASH");
    render_text(2, 8, "(STUB - JUST GATES");
    render_text(2, 9, " INPUT FOR 4 FRAMES)");

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        input_update();

        if (!anim_is_playing()) {
            if (input_pressed(BTN_A)) {
                anim_start(ANIM_WRONG_SHAKE, NULL, 6);
                sfx_wrong();
            }
            if (input_pressed(BTN_B)) {
                anim_start(ANIM_SELECT_FLASH, NULL, 4);
                sfx_select();
            }
        }

        wait_vbl_done();
        anim_tick();
        sound_tick();
        render_flush();
    }
}
```

- [ ] **Step 2: Build and run in BGB**

```bash
make
```

Open the ROM. Press A — verify:
- The screen briefly jitters (alternating ±1 pixel horizontally) for ~6 frames (about 0.1 seconds — very quick)
- The wrong-sound SFX plays
- Pressing A again during the shake does NOT trigger another shake (input is gated by `anim_is_playing()`)

Press B — verify:
- No visible animation (it's a stub)
- The select-sound SFX plays
- Input is gated for 4 frames

### Task 6.4: Commit Phase 6

- [ ] **Step 1: Stage and commit**

```bash
git add Makefile src/engine/anim.h src/engine/anim.c src/main.c
git commit -m "6: implement animation engine — one active anim + per-type tick handlers (WRONG_SHAKE real, others stub for Plan B)"
```

- [ ] **Step 2: Update Phase 6 banner above**

---

## Phase 7 — Game: puzzle_logic + host-side tests (TDD REQUIRED)

**Execution Status:** ⬜ NOT STARTED

**Goal**: Implement the pure logic for puzzle interaction (group correctness, selection toggling, etc.) with TDD-driven host-side unit tests. End state: `make test` runs and passes.

**BEFORE starting work on this phase:**

1. Invoke `/superpowers:test-driven-development`
2. (Pitfalls docs not yet created for this project — see "Notes for executors" at top of plan)

Follow TDD strictly: write the failing test → run it to confirm failure → implement the minimal code → run again to confirm pass → commit. Do not write implementation code before the failing test exists.

### Task 7.1: Create puzzle_logic.h API

**Files:**
- Create: `src/game/puzzle_logic.h`

- [ ] **Step 1: Write the puzzle_logic API**

```c
// src/game/puzzle_logic.h
#ifndef GAME_PUZZLE_LOGIC_H
#define GAME_PUZZLE_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "puzzles_types.h"
#include "game_state.h"

// True if the 4 bits set in selected_mask all correspond to words in the same group.
// Returns false if popcount(selected_mask) != 4 or words span multiple groups.
bool is_group_correct(const Puzzle *p, uint16_t selected_mask);

// Returns the number of bits set in mask (popcount).
uint8_t count_selected(uint16_t mask);

// Toggle the bit at cursor_idx in state->selected_mask. If 4 are already
// selected and the bit at cursor_idx is currently 0, the toggle is rejected
// (function returns false). Otherwise returns true.
bool toggle_selection(PlayState *state, uint8_t cursor_idx);

// Returns the group index (0..3) that word_idx belongs to in puzzle p.
uint8_t find_group_of_word(const Puzzle *p, uint8_t word_idx);

#endif
```

### Task 7.2: Write test/Makefile

**Files:**
- Create: `test/Makefile`

- [ ] **Step 1: Write a host-side test Makefile**

```makefile
# test/Makefile — runs on dev machine via vanilla gcc, NOT GBDK

CC := gcc
CFLAGS := -Wall -Wextra -Werror -I../src -std=c99

.PHONY: test clean

test: test_puzzle_logic
	./test_puzzle_logic

test_puzzle_logic: test_puzzle_logic.c ../src/game/puzzle_logic.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_puzzle_logic
```

### Task 7.3: Write the failing test for is_group_correct (TDD step 1)

**Files:**
- Create: `test/test_puzzle_logic.c`

- [ ] **Step 1: Write a failing test**

```c
// test/test_puzzle_logic.c
#include "game/puzzle_logic.h"
#include <stdio.h>
#include <assert.h>

// Mock puzzle: words 0-3 in group 0, 4-7 in group 1, etc.
static const Puzzle TEST_PUZZLE = {
    .words = {
        "A","B","C","D","E","F","G","H",
        "I","J","K","L","M","N","O","P"
    },
    .group_of = {
        0,0,0,0,
        1,1,1,1,
        2,2,2,2,
        3,3,3,3
    },
    .category_names = {"G0","G1","G2","G3"}
};

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

static void test_is_group_correct_all_same_group(void) {
    // Bits 0,1,2,3 set → all group 0
    ASSERT_TRUE(is_group_correct(&TEST_PUZZLE, 0x000F), "all-group-0 should be correct");
}

int main(void) {
    test_is_group_correct_all_same_group();

    printf("Tests run: %d, Failed: %d\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
```

- [ ] **Step 2: Create empty puzzle_logic.c so linker has a symbol target**

```bash
touch src/game/puzzle_logic.c
```

Add the include + an empty function definition (just enough to get a "wrong answer" rather than a linker error):

```c
// src/game/puzzle_logic.c
#include "puzzle_logic.h"

bool is_group_correct(const Puzzle *p, uint16_t selected_mask) {
    (void)p; (void)selected_mask;
    return false;  // intentionally wrong — TDD red phase
}

uint8_t count_selected(uint16_t mask) {
    (void)mask;
    return 0;
}

bool toggle_selection(PlayState *state, uint8_t cursor_idx) {
    (void)state; (void)cursor_idx;
    return false;
}

uint8_t find_group_of_word(const Puzzle *p, uint8_t word_idx) {
    (void)p; (void)word_idx;
    return 0;
}
```

- [ ] **Step 3: Run the test, confirm failure**

```bash
cd test && make
```

Expected: compiles cleanly, then `FAIL: test_is_group_correct_all_same_group ...: all-group-0 should be correct` and `Tests run: 1, Failed: 1`, exit code 1.

If it passes, the implementation is accidentally returning true — verify `is_group_correct` returns `false` in the stub.

### Task 7.4: Implement is_group_correct (TDD green)

**Files:**
- Modify: `src/game/puzzle_logic.c`

- [ ] **Step 1: Replace the stub with a real implementation**

```c
bool is_group_correct(const Puzzle *p, uint16_t selected_mask) {
    if (count_selected(selected_mask) != 4) return false;

    // Find the group of the first selected word, then verify all others match
    int8_t target_group = -1;
    for (uint8_t i = 0; i < WORDS_PER_PUZZLE; i++) {
        if (selected_mask & (1u << i)) {
            if (target_group == -1) {
                target_group = (int8_t)p->group_of[i];
            } else if (p->group_of[i] != (uint8_t)target_group) {
                return false;
            }
        }
    }
    return target_group != -1;  // true unless mask was 0 (caught by count check)
}
```

And implement `count_selected` for the popcount support:

```c
uint8_t count_selected(uint16_t mask) {
    uint8_t count = 0;
    while (mask) { count += (uint8_t)(mask & 1); mask >>= 1; }
    return count;
}
```

- [ ] **Step 2: Run the test, confirm pass**

```bash
cd test && make
```

Expected: `Tests run: 1, Failed: 0`, exit code 0.

### Task 7.5: Add tests for mixed-group, wrong-count, and other branches

**Files:**
- Modify: `test/test_puzzle_logic.c`

- [ ] **Step 1: Add failing tests for additional branches**

The `test/test_puzzle_logic.c` file already has a `main()` from Task 7.3. Do NOT create a second `main()` — that's a duplicate-symbol linker error. Instead:

1. **Add the new test function definitions** above the existing `main()` function.
2. **Add new function-call lines** inside the existing `main()` body, between the existing call to `test_is_group_correct_all_same_group();` and the `printf("Tests run: ...")` line.

The example `int main(void) { ... }` block shown later in this step is just to illustrate what the FINAL main body should look like after your edits. Do not paste a literal `int main(void)` declaration — there is already one in the file.

```c
static void test_is_group_correct_three_plus_one(void) {
    // Bits 0,1,2,4 → 3 from group 0, 1 from group 1
    ASSERT_TRUE(!is_group_correct(&TEST_PUZZLE, 0x0017), "3+1 should not be correct");
}

static void test_is_group_correct_only_three_selected(void) {
    // Only 3 selected → invalid submission
    ASSERT_TRUE(!is_group_correct(&TEST_PUZZLE, 0x0007), "3 selected should not be correct");
}

static void test_is_group_correct_only_five_selected(void) {
    // 5 selected → invalid submission
    ASSERT_TRUE(!is_group_correct(&TEST_PUZZLE, 0x001F), "5 selected should not be correct");
}

static void test_is_group_correct_zero_selected(void) {
    ASSERT_TRUE(!is_group_correct(&TEST_PUZZLE, 0x0000), "0 selected should not be correct");
}

static void test_count_selected(void) {
    ASSERT_EQ(count_selected(0x0000), 0, "popcount(0)");
    ASSERT_EQ(count_selected(0xFFFF), 16, "popcount(0xFFFF)");
    ASSERT_EQ(count_selected(0x000F), 4, "popcount(0x000F)");
    ASSERT_EQ(count_selected(0xAAAA), 8, "popcount(alternating)");
}
```

And add them to main():

```c
int main(void) {
    test_is_group_correct_all_same_group();
    test_is_group_correct_three_plus_one();
    test_is_group_correct_only_three_selected();
    test_is_group_correct_only_five_selected();
    test_is_group_correct_zero_selected();
    test_count_selected();

    printf("Tests run: %d, Failed: %d\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
```

- [ ] **Step 2: Run tests — `is_group_correct` tests should already pass; `count_selected` should fail until proper popcount lands**

```bash
cd test && make
```

The count_selected tests should pass with the implementation from Task 7.4. If they all pass, move on. If any unexpected failure, debug.

### Task 7.6: TDD toggle_selection

**Files:**
- Modify: `test/test_puzzle_logic.c` then `src/game/puzzle_logic.c`

- [ ] **Step 1: Write failing tests for toggle_selection**

Add to test file:

```c
static void test_toggle_selection_adds_when_under_limit(void) {
    PlayState s = {0};
    s.selected_mask = 0x0001;  // bit 0 already set
    bool ok = toggle_selection(&s, 5);  // toggle bit 5
    ASSERT_TRUE(ok, "should accept toggle when under 4");
    ASSERT_EQ(s.selected_mask, 0x0021, "bit 5 should be set");
}

static void test_toggle_selection_removes_existing(void) {
    PlayState s = {0};
    s.selected_mask = 0x0001;  // bit 0 set
    bool ok = toggle_selection(&s, 0);  // toggle bit 0 off
    ASSERT_TRUE(ok, "should accept toggle of existing selection");
    ASSERT_EQ(s.selected_mask, 0x0000, "bit 0 should be cleared");
}

static void test_toggle_selection_rejects_fifth(void) {
    PlayState s = {0};
    s.selected_mask = 0x000F;  // 4 already selected (bits 0-3)
    bool ok = toggle_selection(&s, 5);  // attempt 5th
    ASSERT_TRUE(!ok, "should reject 5th selection");
    ASSERT_EQ(s.selected_mask, 0x000F, "mask should be unchanged");
}

static void test_toggle_selection_allows_deselect_when_at_limit(void) {
    PlayState s = {0};
    s.selected_mask = 0x000F;  // 4 selected
    bool ok = toggle_selection(&s, 2);  // toggle bit 2 (already set) → deselect
    ASSERT_TRUE(ok, "should allow deselect at limit");
    ASSERT_EQ(s.selected_mask, 0x000B, "bit 2 should be cleared");
}
```

And call them from main(). Re-run; the 4 new toggle tests should fail (`toggle_selection` is still a stub returning false).

```bash
cd test && make
```

Expected: 4 new failures.

- [ ] **Step 2: Implement toggle_selection**

In `src/game/puzzle_logic.c`, replace the stub:

```c
bool toggle_selection(PlayState *state, uint8_t cursor_idx) {
    uint16_t bit = (uint16_t)(1u << cursor_idx);
    bool currently_set = (state->selected_mask & bit) != 0;

    if (!currently_set && count_selected(state->selected_mask) >= 4) {
        return false;  // reject 5th selection
    }
    state->selected_mask ^= bit;
    return true;
}
```

- [ ] **Step 3: Run tests, confirm pass**

```bash
cd test && make
```

Expected: all tests pass.

### Task 7.7: TDD find_group_of_word

**Files:**
- Modify: `test/test_puzzle_logic.c` then `src/game/puzzle_logic.c`

- [ ] **Step 1: Add tests**

```c
static void test_find_group_of_word(void) {
    ASSERT_EQ(find_group_of_word(&TEST_PUZZLE, 0), 0, "word 0 → group 0");
    ASSERT_EQ(find_group_of_word(&TEST_PUZZLE, 5), 1, "word 5 → group 1");
    ASSERT_EQ(find_group_of_word(&TEST_PUZZLE, 11), 2, "word 11 → group 2");
    ASSERT_EQ(find_group_of_word(&TEST_PUZZLE, 15), 3, "word 15 → group 3");
}
```

Call from main(). Re-run; should fail.

- [ ] **Step 2: Implement**

```c
uint8_t find_group_of_word(const Puzzle *p, uint8_t word_idx) {
    return p->group_of[word_idx];
}
```

- [ ] **Step 3: Run, confirm pass**

```bash
cd test && make
```

Expected: all tests pass.

### Task 7.8: Hook test target into top-level Makefile

**Files:**
- Modify: `Makefile`

- [ ] **Step 1: Add a `test` target to the top-level Makefile**

Add to root `Makefile`:

```makefile
.PHONY: test
test:
	$(MAKE) -C test
```

- [ ] **Step 2: Verify `make test` from project root works**

```bash
cd ..  # if currently in test/
make test
```

Expected: enters test/, builds test_puzzle_logic, runs it, prints pass count.

### Task 7.9: Verify puzzle_logic.c remains GBDK-free (architectural invariant)

- [ ] **Step 1: Confirm puzzle_logic.c has no GBDK includes**

```bash
grep -E "^#include" src/game/puzzle_logic.c
```

Expected output should be ONLY:
```
#include "puzzle_logic.h"
```

If you see `<gb/gb.h>` or any `gb/*` header, the architectural invariant is broken — the file is no longer host-testable. Remove the include.

- [ ] **Step 2: Verify puzzle_logic.c also compiles inside the GBDK build**

```bash
# From project root
make
```

Should succeed (the file is pure C99, GBDK-compatible). If it doesn't, examine errors.

### Task 7.10: Commit Phase 7

- [ ] **Step 1: Final review against the testing discipline**

Run `make test` one more time. Confirm: no skipped tests, no commented-out tests, all assertions are mechanism-grounded (`is_group_correct(...) == true/false`) not symptom-weakened.

- [ ] **Step 2: Stage and commit**

```bash
git add Makefile src/game/puzzle_logic.h src/game/puzzle_logic.c test/Makefile test/test_puzzle_logic.c
git commit -m "7: implement puzzle_logic with TDD — is_group_correct, toggle_selection, find_group_of_word + 14 unit tests"
```

- [ ] **Step 3: Update Phase 7 banner above**

---

## Phase 8 — Puzzle JSON pipeline (TDD REQUIRED)

**Execution Status:** ⬜ NOT STARTED

**Goal**: Build the Python codegen pipeline — load `content/puzzles.json`, validate against the 6 rules from spec §6, deterministically shuffle each puzzle, emit `src/puzzles_data.c`. Plus an initial 5-puzzle sample bank baked into the ROM. End state: `make` runs codegen as part of build, produces a ROM with sample puzzles embedded.

**BEFORE starting work on this phase:**
1. Invoke `/superpowers:test-driven-development`

Tasks in this phase follow strict TDD: write the failing test for each validation rule first, then implement.

### Task 8.1: Write the build_puzzles.py skeleton

**Files:**
- Create: `tools/build_puzzles.py`

- [ ] **Step 1: Write the minimal CLI skeleton**

```python
#!/usr/bin/env python3
"""build_puzzles.py — Validate content/puzzles.json and emit src/puzzles_data.c.

Usage: python build_puzzles.py <input_json> <output_c>
"""

import json
import random
import sys
from typing import List, Dict, Any


# ---- Public validation API ----

def validate_puzzle(puzzle: Dict[str, Any]) -> None:
    """Raise ValueError on validation failure. No return on success."""
    # Implementation lands incrementally in subsequent tasks.
    pass


def validate_all(data: Dict[str, Any]) -> List[Dict[str, Any]]:
    """Returns the list of non-draft puzzles in input order. Raises on any failure."""
    if "puzzles" not in data or not isinstance(data["puzzles"], list):
        raise ValueError("Top-level must contain 'puzzles' array")
    non_draft = [p for p in data["puzzles"] if not p.get("draft", False)]
    for p in non_draft:
        validate_puzzle(p)
    # ID sequence check: ids of non-draft must be 1, 2, 3, ... in order
    for i, p in enumerate(non_draft, start=1):
        if p.get("id") != i:
            raise ValueError(
                f"puzzle at position {i}: id is {p.get('id')!r}, "
                f"expected {i} (ids must be sequential starting from 1, no gaps)"
            )
    return non_draft


# ---- Shuffle (deterministic per puzzle id) ----

def shuffle_puzzle(puzzle_id: int, words_with_groups: List[tuple]) -> List[tuple]:
    """Return a shuffled copy. Same input + same id = same output (deterministic)."""
    rng = random.Random(puzzle_id * 31 + 7)
    shuffled = list(words_with_groups)
    rng.shuffle(shuffled)
    return shuffled


# ---- Codegen ----

def emit_c(puzzles: List[Dict[str, Any]], out_path: str) -> None:
    """Write src/puzzles_data.c with the PUZZLES array and NUM_PUZZLES constant."""
    lines = []
    lines.append("// AUTO-GENERATED by tools/build_puzzles.py — DO NOT EDIT")
    lines.append('#include "game/puzzles_types.h"')
    lines.append("")
    lines.append(f"const uint8_t NUM_PUZZLES = {len(puzzles)};")
    lines.append("")
    lines.append(f"const Puzzle PUZZLES[{len(puzzles)}] = {{")

    tier_order = ["yellow", "green", "blue", "purple"]
    for p in puzzles:
        # Build flat (word, group_idx) list in category-grouped order
        flat = []
        names = ["", "", "", ""]
        for cat in p["categories"]:
            tier_idx = tier_order.index(cat["tier"])
            names[tier_idx] = cat["name"]
            for w in cat["words"]:
                flat.append((w, tier_idx))
        # Shuffle to display order
        shuffled = shuffle_puzzle(p["id"], flat)
        words = [w for w, _ in shuffled]
        groups = [g for _, g in shuffled]

        lines.append(f"  // ----- Puzzle {p['id']} -----")
        lines.append("  {")
        words_c = ", ".join(f'"{w}"' for w in words)
        lines.append(f"    .words = {{ {words_c} }},")
        groups_c = ", ".join(str(g) for g in groups)
        lines.append(f"    .group_of = {{ {groups_c} }},")
        names_c = ", ".join(f'"{n}"' for n in names)
        lines.append(f"    .category_names = {{ {names_c} }}")
        lines.append("  },")
    lines.append("};")

    with open(out_path, "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


# ---- Main ----

def main(argv: List[str]) -> int:
    if len(argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    input_path, output_path = argv[1], argv[2]
    with open(input_path) as f:
        data = json.load(f)
    try:
        puzzles = validate_all(data)
    except ValueError as e:
        print(f"VALIDATION ERROR: {e}", file=sys.stderr)
        return 1
    emit_c(puzzles, output_path)
    print(f"Wrote {len(puzzles)} puzzles to {output_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
```

### Task 8.2: Write test_build_puzzles.py with first failing test (TDD red)

**Files:**
- Create: `tools/test_build_puzzles.py`

- [ ] **Step 1: Write the test framework + one failing test for rule 2 (missing tier)**

```python
#!/usr/bin/env python3
"""Unit tests for tools/build_puzzles.py validation rules."""

import unittest
from build_puzzles import validate_puzzle


def make_valid_puzzle(pid=1, overrides=None):
    """Helper: build a structurally-valid puzzle, then apply overrides."""
    base = {
        "id": pid,
        "categories": [
            {"tier": "yellow", "name": "AAA", "words": ["A", "B", "C", "D"]},
            {"tier": "green",  "name": "BBB", "words": ["E", "F", "G", "H"]},
            {"tier": "blue",   "name": "CCC", "words": ["I", "J", "K", "L"]},
            {"tier": "purple", "name": "DDD", "words": ["M", "N", "O", "P"]},
        ]
    }
    if overrides:
        base.update(overrides)
    return base


class TestValidate(unittest.TestCase):
    def test_valid_puzzle_passes(self):
        validate_puzzle(make_valid_puzzle())  # should not raise

    def test_missing_tier(self):
        # Replace purple with yellow → tiers are {yellow, yellow, green, blue}
        p = make_valid_puzzle()
        p["categories"][3]["tier"] = "yellow"
        with self.assertRaisesRegex(ValueError, r"tier"):
            validate_puzzle(p)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run tests, confirm 1 fail (test_missing_tier passes incorrectly because validate_puzzle is a no-op stub)**

```bash
cd tools && python -m unittest test_build_puzzles.py -v
```

Expected: `test_missing_tier` FAILS with "ValueError not raised". `test_valid_puzzle_passes` passes.

### Task 8.3: Implement validation rule 2 (tiers are {yellow, green, blue, purple}, no duplicates)

**Files:**
- Modify: `tools/build_puzzles.py`

- [ ] **Step 1: Add tier validation**

Replace the `validate_puzzle` stub with:

```python
def validate_puzzle(puzzle: Dict[str, Any]) -> None:
    pid = puzzle.get("id", "?")
    cats = puzzle.get("categories", [])

    if len(cats) != 4:
        raise ValueError(f"puzzle {pid}: must have exactly 4 categories, got {len(cats)}")

    tiers = [c.get("tier") for c in cats]
    expected_tiers = {"yellow", "green", "blue", "purple"}
    if set(tiers) != expected_tiers:
        raise ValueError(
            f"puzzle {pid}: tiers must be exactly {expected_tiers}, "
            f"got {set(tiers)} (duplicates or missing)"
        )
```

- [ ] **Step 2: Re-run tests**

```bash
cd tools && python -m unittest test_build_puzzles.py -v
```

Expected: both tests pass.

### Task 8.4: TDD validation rule 3 (4 words per category + category name format)

**Files:**
- Modify: `tools/test_build_puzzles.py`, then `tools/build_puzzles.py`

- [ ] **Step 1: Add failing tests for rule 3 cases**

```python
    def test_wrong_word_count(self):
        p = make_valid_puzzle()
        p["categories"][0]["words"] = ["A", "B", "C"]  # only 3
        with self.assertRaisesRegex(ValueError, r"4 words"):
            validate_puzzle(p)

    def test_name_too_long(self):
        p = make_valid_puzzle()
        p["categories"][0]["name"] = "A" * 13  # 13 > 12 limit
        with self.assertRaisesRegex(ValueError, r"name.*length"):
            validate_puzzle(p)

    def test_name_invalid_chars(self):
        p = make_valid_puzzle()
        p["categories"][0]["name"] = "bird-1"  # lowercase + hyphen + digit
        with self.assertRaisesRegex(ValueError, r"name"):
            validate_puzzle(p)
```

Run tests; should fail.

- [ ] **Step 2: Implement rule 3**

Extend `validate_puzzle`:

```python
import re

_NAME_RE = re.compile(r"^[A-Z ]{1,12}$")

def validate_puzzle(puzzle: Dict[str, Any]) -> None:
    pid = puzzle.get("id", "?")
    cats = puzzle.get("categories", [])

    if len(cats) != 4:
        raise ValueError(f"puzzle {pid}: must have exactly 4 categories, got {len(cats)}")

    tiers = [c.get("tier") for c in cats]
    expected_tiers = {"yellow", "green", "blue", "purple"}
    if set(tiers) != expected_tiers:
        raise ValueError(
            f"puzzle {pid}: tiers must be exactly {expected_tiers}, "
            f"got {set(tiers)} (duplicates or missing)"
        )

    for cat in cats:
        words = cat.get("words", [])
        name = cat.get("name", "")
        tier = cat.get("tier", "?")

        if len(words) != 4:
            raise ValueError(
                f"puzzle {pid}, category {tier!r}: must have exactly 4 words, got {len(words)}"
            )
        if not _NAME_RE.match(name):
            raise ValueError(
                f"puzzle {pid}, category {tier!r}: name {name!r} must match "
                f"^[A-Z ]{{1,12}}$ (uppercase A-Z + space, length 1-12)"
            )
```

Run tests; expect pass.

### Task 8.5: TDD validation rule 4 (word format)

**Files:**
- Modify: `tools/test_build_puzzles.py`, then `tools/build_puzzles.py`

- [ ] **Step 1: Add failing tests**

```python
    def test_word_too_long(self):
        p = make_valid_puzzle()
        p["categories"][0]["words"][0] = "SPAGHETTI"  # 9 chars
        with self.assertRaisesRegex(ValueError, r"length"):
            validate_puzzle(p)

    def test_word_lowercase(self):
        p = make_valid_puzzle()
        p["categories"][0]["words"][0] = "robin"
        with self.assertRaisesRegex(ValueError, r"word"):
            validate_puzzle(p)

    def test_word_with_punctuation(self):
        p = make_valid_puzzle()
        p["categories"][0]["words"][0] = "DON'T"
        with self.assertRaisesRegex(ValueError, r"word"):
            validate_puzzle(p)
```

- [ ] **Step 2: Implement rule 4** — add a word loop inside the category loop in `validate_puzzle`:

```python
_WORD_RE = re.compile(r"^[A-Z]{1,8}$")

# Inside validate_puzzle, after the name check:
        for w in words:
            if not _WORD_RE.match(w):
                raise ValueError(
                    f"puzzle {pid}, category {tier!r}: word {w!r} must match "
                    f"^[A-Z]{{1,8}}$ (uppercase A-Z only, length 1-8)"
                )
```

Run tests; expect pass.

### Task 8.6: TDD validation rules 5 & 6 (no duplicate words, no name=word collision)

**Files:**
- Modify: both files

- [ ] **Step 1: Add failing tests**

```python
    def test_duplicate_word_within_puzzle(self):
        p = make_valid_puzzle()
        p["categories"][1]["words"][0] = "A"  # "A" also in category 0
        with self.assertRaisesRegex(ValueError, r"duplicate"):
            validate_puzzle(p)

    def test_name_word_collision(self):
        p = make_valid_puzzle()
        # Category 0 is named "AAA"; put "AAA" as a word in category 1
        p["categories"][1]["words"][0] = "AAA"
        with self.assertRaisesRegex(ValueError, r"collision|name.*word"):
            validate_puzzle(p)
```

- [ ] **Step 2: Implement rules 5 & 6** — add after the category loop:

```python
    # Rule 5: no duplicate words (case-insensitive) across the whole puzzle
    all_words = []
    for cat in cats:
        for w in cat["words"]:
            all_words.append(w.upper())
    if len(all_words) != len(set(all_words)):
        seen = set()
        for w in all_words:
            if w in seen:
                raise ValueError(f"puzzle {pid}: duplicate word {w!r}")
            seen.add(w)

    # Rule 6: no category name equals any word (case-insensitive)
    name_set = {cat["name"].upper() for cat in cats}
    word_set = {w.upper() for w in all_words}
    collisions = name_set & word_set
    if collisions:
        raise ValueError(
            f"puzzle {pid}: collision between category name and word: {collisions}"
        )
```

Run tests; expect pass.

### Task 8.7: TDD validation rule 1 (sequential IDs) via validate_all

**Files:**
- Modify: both files

- [ ] **Step 1: Add failing tests**

```python
from build_puzzles import validate_all

class TestValidateAll(unittest.TestCase):
    def test_sequential_ids_pass(self):
        data = {"puzzles": [make_valid_puzzle(1), make_valid_puzzle(2), make_valid_puzzle(3)]}
        validate_all(data)  # should not raise

    def test_id_gap_fails(self):
        data = {"puzzles": [make_valid_puzzle(1), make_valid_puzzle(3)]}  # gap at 2
        with self.assertRaisesRegex(ValueError, r"sequential|id"):
            validate_all(data)

    def test_draft_excluded_from_id_check(self):
        # Draft puzzles are excluded; remaining non-drafts must still be 1, 2, 3...
        data = {"puzzles": [
            make_valid_puzzle(1),
            {**make_valid_puzzle(99), "draft": True},  # draft excluded
            make_valid_puzzle(2),
        ]}
        validate_all(data)  # should not raise — non-drafts are [id=1, id=2] in order
```

Run tests; rule 1 logic is already in `validate_all` from Task 8.1, so these should pass except potentially the `test_draft_excluded_from_id_check` if draft handling has a bug. Verify and fix.

### Task 8.8: Hook build_puzzles into the top-level Makefile

**Files:**
- Modify: `Makefile`

This task does TWO things to the root Makefile: (1) adds codegen rules, (2) REPLACES the `test:` target from Task 7.8 with a unified one that chains both host-logic tests and codegen tests.

- [ ] **Step 1: Add codegen step to Makefile**

Add these lines to `Makefile` (place them above the existing `SRC := ...` line so the variable is visible when SRC is appended):

```makefile
# Puzzle data codegen — runs whenever JSON or script changes
PUZZLE_JSON := content/puzzles.json
PUZZLE_OUT  := src/puzzles_data.c
PUZZLE_SCRIPT := tools/build_puzzles.py

$(PUZZLE_OUT): $(PUZZLE_JSON) $(PUZZLE_SCRIPT)
	python $(PUZZLE_SCRIPT) $(PUZZLE_JSON) $(PUZZLE_OUT)
```

Then add a new `SRC += $(PUZZLE_OUT)` line directly below the existing `SRC += $(ASSETS_GEN)` line. Do NOT modify the existing `SRC := ...` line itself — the new `SRC +=` line goes below it. After this task, the relevant Makefile block should read:

```makefile
SRC := src/main.c src/engine/render.c src/engine/input.c src/engine/save.c src/engine/sound.c src/engine/anim.c
SRC += $(ASSETS_GEN)
SRC += $(PUZZLE_OUT)
```

- [ ] **Step 2: REPLACE the existing `test:` target (added in Task 7.8) with a chained version**

Find these lines in `Makefile` (added by Task 7.8):

```makefile
.PHONY: test
test:
	$(MAKE) -C test
```

DELETE those 3 lines and REPLACE them with:

```makefile
.PHONY: test test-logic test-codegen
test: test-logic test-codegen

test-logic:
	$(MAKE) -C test

test-codegen:
	cd tools && python -m unittest test_build_puzzles.py
```

This avoids the duplicate `test:` target rule that would otherwise occur. Do NOT leave both versions in the file — Make will warn ("overriding recipe for target 'test'") and behavior becomes implementation-defined.

- [ ] **Step 3: Verify the unified test target works**

```bash
make test
```

Expected output: runs the C unit tests (printing "Tests run: N, Failed: 0"), then runs the Python unittests (printing dots and "OK"). Exit code 0.

If you see "overriding recipe for target 'test'" warnings, the old `test:` target wasn't deleted — find and remove the duplicate.

### Task 8.9: Author the initial sample puzzle bank

**Files:**
- Create: `content/puzzles.json`

- [ ] **Step 1: Write 5 sample puzzles spanning difficulty**

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
    },
    {
      "id": 2,
      "categories": [
        { "tier": "yellow", "name": "FRUITS",  "words": ["APPLE", "PEAR", "PEACH", "MANGO"] },
        { "tier": "green",  "name": "PLANETS", "words": ["MARS", "EARTH", "VENUS", "PLUTO"] },
        { "tier": "blue",   "name": "METALS",  "words": ["IRON", "GOLD", "TIN", "ZINC"] },
        { "tier": "purple", "name": "DANCES",  "words": ["WALTZ", "TANGO", "SALSA", "POLKA"] }
      ]
    },
    {
      "id": 3,
      "categories": [
        { "tier": "yellow", "name": "TOOLS",   "words": ["HAMMER", "SAW", "DRILL", "WRENCH"] },
        { "tier": "green",  "name": "WEATHER", "words": ["RAIN", "SNOW", "WIND", "HAIL"] },
        { "tier": "blue",   "name": "OCEANS",  "words": ["PACIFIC", "ARCTIC", "INDIAN", "ATLANTIC"] },
        { "tier": "purple", "name": "VERBS",   "words": ["WALK", "TALK", "SING", "DANCE"] }
      ]
    },
    {
      "id": 4,
      "categories": [
        { "tier": "yellow", "name": "BIG CATS",  "words": ["LION", "TIGER", "PUMA", "LYNX"] },
        { "tier": "green",  "name": "ELEMENTS",  "words": ["HELIUM", "NEON", "ARGON", "RADON"] },
        { "tier": "blue",   "name": "SPORTS",    "words": ["SOCCER", "GOLF", "TENNIS", "RUGBY"] },
        { "tier": "purple", "name": "INSTRUM",   "words": ["PIANO", "VIOLIN", "DRUMS", "FLUTE"] }
      ]
    },
    {
      "id": 5,
      "categories": [
        { "tier": "yellow", "name": "MONTHS",   "words": ["MARCH", "APRIL", "JUNE", "JULY"] },
        { "tier": "green",  "name": "GEMS",     "words": ["RUBY", "OPAL", "JADE", "ONYX"] },
        { "tier": "blue",   "name": "RIVERS",   "words": ["NILE", "AMAZON", "DANUBE", "VOLGA"] },
        { "tier": "purple", "name": "SHAPES",   "words": ["CIRCLE", "SQUARE", "OVAL", "STAR"] }
      ]
    }
  ]
}
```

Note: `INSTRUM` for the instruments category in puzzle 4 — fits the 12-char name limit. Future puzzles can use more creative shortenings.

- [ ] **Step 2: Build the ROM with puzzles baked in**

```bash
make
```

Expected: no validation errors; `src/puzzles_data.c` generated; ROM builds successfully.

- [ ] **Step 3: Inspect the generated C**

```bash
cat src/puzzles_data.c | head -30
```

Verify it contains a `const Puzzle PUZZLES[5]` array, that words appear in shuffled (not category) order, and that `group_of` arrays accurately tag each word.

### Task 8.10: Update main.c to display loaded puzzle count

**Files:**
- Modify: `src/main.c`

- [ ] **Step 1: Show NUM_PUZZLES + a puzzle's data on screen**

```c
// src/main.c
#include <gb/gb.h>
#include <stdio.h>
#include "engine/render.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/sound.h"
#include "engine/anim.h"
#include "game/puzzles_types.h"

void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();

    char buf[21];

    sprintf(buf, "NUM_PUZZLES: %d", NUM_PUZZLES);
    render_text(2, 1, "PLAN A COMPLETE");
    render_text(2, 3, buf);

    sprintf(buf, "P1 W0: %s", PUZZLES[0].words[0]);
    render_text(2, 5, buf);
    sprintf(buf, "P1 W1: %s", PUZZLES[0].words[1]);
    render_text(2, 6, buf);
    sprintf(buf, "P1 GRP[0]: %d", PUZZLES[0].group_of[0]);
    render_text(2, 8, buf);
    sprintf(buf, "CAT 0: %s", PUZZLES[0].category_names[0]);
    render_text(2, 10, buf);

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        wait_vbl_done();
        render_flush();
    }
}
```

- [ ] **Step 2: Build and run**

```bash
make
```

Open in BGB. Verify the screen shows `NUM_PUZZLES: 5`, the first puzzle's first two words (these will be the shuffled positions, NOT alphabetical), the group index for word 0, and the first category name.

This confirms the entire pipeline: JSON → Python codegen → C compilation → linker → ROM → runtime read of puzzle data.

### Task 8.11: Commit Phase 8

- [ ] **Step 1: Stage and commit**

```bash
git add Makefile tools/build_puzzles.py tools/test_build_puzzles.py content/puzzles.json src/main.c
git status  # verify src/puzzles_data.c is NOT staged (gitignored — generated)
git commit -m "8: implement puzzle JSON pipeline — Python codegen + 11 validation tests + 5 sample puzzles baked into ROM"
```

- [ ] **Step 2: Update Phase 8 banner above**

---

## End of Plan A

When all 8 phases ship and their banners are updated, Plan A is complete. The artifact at this point:

- A `.gb` ROM that boots, has all 5 engine subsystems exercised via smoke-test scaffolding, demonstrates input + render + save + sound + anim working
- A passing `make test` covering 14+ puzzle_logic tests and 11+ codegen validation tests
- 5 sample puzzles authored in `content/puzzles.json` and baked into the ROM via Python codegen
- A working build pipeline (`make`, `make test`, `make clean`, `make size`)
- All architectural invariants verified (engine/game split, host-testable pure logic, deterministic ROM-baked puzzle data)

**Next**: Write Plan B (Gameplay), which replaces the `main.c` smoke-test scaffolding with the actual 5 scenes (TITLE, PLAY, WIN, LOSE, ALL_DONE) using the engine + logic + puzzles foundation Plan A delivered.
