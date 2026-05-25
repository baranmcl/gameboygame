# GBC Color Support + Title Chiptune Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Game Boy Color palette support (yellow/green/blue/purple tier bars + accent-colored title banner) that gracefully degrades to the existing greyscale rendering on the original DMG, AND a short 8-bar CH1-only chiptune that plays on the title screen.

**Architecture:** Two independent subsystems delivered together as v1.1 release:

1. **Color**: Cart header flagged GBC-aware (works on both DMG and GBC). At boot we set up 5 GBC background palettes (default greyscale + 4 NYT-faithful tier colors). When scenes render tier bars they additionally write the per-tile attribute byte in VRAM bank 1 to assign the matching palette. On DMG the attribute writes are no-ops (DMG has no VRAM bank 1 / no per-tile palette indexing), so the visual is unchanged from v1.0.

2. **Music**: New `src/engine/music.{h,c}` module. Pattern-based player: a `MusicTrack` is an array of `{note_index, duration_frames}` records. `music_tick()` runs from the VBlank ISR, advances the playhead, and writes NR13/NR14 when a new note starts. CH1 (square 1) reserved for music; SFX continue to use CH2/CH4 so they don't clash with the title theme. Scene_title starts the track on init and stops it on teardown.

**Tech Stack:** GBDK-2020 + SDCC (existing). GBC palette manipulation via `BCPS_REG` / `BCPD_REG` (or GBDK helpers `set_bkg_palette`). Per-tile attribute writes via `VBK_REG=1` then `set_bkg_tiles` (GBC only — DMG ignores writes to bank 1). Music uses the same `gb/hardware.h` channel registers as existing `sound.c`. No new external dependencies.

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

**Overall:** 4/7 phases shipped (Phase 4 as scope cut). Phases 5-7 in progress.

| Phase | Status | Ship SHA(s) | Notes |
|---|---|---|---|
| 1 — Cart header GBC-aware + runtime detection | ✅ Shipped | `b4a2150`, `2e3a70f` | 2026-05-25; -Wm-yc + is_gbc() helper |
| 2 — GBC palette infrastructure | ✅ Shipped | `42f606d`, `ee1733e`, `c40428c` | 2026-05-25; 5 palettes (originally 6, then trimmed in Phase 4) + attribute API |
| 3 — Apply tier colors to gameplay scenes | ✅ Shipped | `7f847f2`, `772b4e8`, `4314638` | 2026-05-25; all 3 scenes render tier colors on GBC, unchanged on DMG |
| 4 — Title banner accent color | ✅ Shipped (scope cut) | `6df4f11` | 2026-05-25; user preferred unstylized title — reverted; tier colors stay as the visual signature |
| 5 — Music player engine | ⬜ Not started | — | `src/engine/music.{h,c}` + host tests |
| 6 — Title theme + integration | ⬜ Not started | — | 8-bar CH1 loop, scene_title wiring |
| 7 — Final verification + v1.1 release | ⬜ Not started | — | DMG/GBC verify, README, tag, asset upload |

---

## Architecture Notes (read before any phase)

### The Game Boy hardware difference that drives everything

- **DMG** (original Game Boy): 4-shade greyscale. One background palette via `BGP_REG`. No per-tile palette assignment. No VRAM bank 1.
- **GBC** (Game Boy Color): 32 colors visible simultaneously across 8 background palettes × 4 colors. Per-tile palette assignment via the VRAM bank 1 attribute map (1 byte per tile, mirroring the tilemap layout). DMG-format cartridges run unchanged on GBC in monochrome — the GBC just applies a default palette inference.
- **GBC-aware cartridge** (what we're building): cart header byte `0x143` set to `0x80` instead of `0x00`. The CPU still works in DMG-compatible mode on the original Game Boy (no GBC features available), but on GBC the system exposes the color hardware to the ROM.
- **What does NOT change**: code, tile shapes, tilemap layout, font, all gameplay logic. Only palette setup at boot and per-tile attribute writes when drawing tier bars / title banner.

### Detecting GBC at runtime

GBDK provides the `_cpu` global byte (`#include <gb/cpu.h>` or just `<gb/gb.h>`):
- `_cpu == 0x01` → DMG (original Game Boy)
- `_cpu == 0x11` → CGB (Game Boy Color)
- `_cpu == 0xFF` → MGB (Game Boy Pocket — treat as DMG)

We wrap this in an `is_gbc()` inline helper for readability and one-place-to-change. All color code paths check `is_gbc()` and no-op on DMG.

### Why palette setup happens once at boot, not per-scene

GBC palettes live in dedicated palette RAM (not VRAM) and persist until rewritten. Setting them once in `render_init()` means scenes don't need to know about palettes — they just write tile-attribute bytes that index into already-loaded palettes. This keeps scene code identical in shape to today's (which already calls `render_set_tile`).

### Music channel reservation

`sound.c` currently uses:
- **CH1 (NR10-NR14)**: multi-step SFX (correct/wrong/win/lose/skip — arpeggios + sustained notes)
- **CH2 (NR21-NR24)**: short tap SFX (move/select/deselect)
- **CH4 (NR41-NR44)**: noise SFX (reject)

On the title screen, only `sfx_move`, `sfx_select`, `sfx_deselect` fire — all CH2. So music can use CH1 exclusively on TITLE without conflict. We do NOT need to refactor `sound.c` channel assignments.

When the player leaves TITLE (entering PLAY via NEW GAME / CONTINUE / RESTART), `scene_title.teardown` stops the music. From there on, CH1 is available for the SFX scheduler again. No music plays during gameplay.

---

## Phase 1 — Cart header GBC-aware + runtime detection

**Execution Status:** ✅ SHIPPED at `b4a2150` (Makefile) + `2e3a70f` (is_gbc) on 2026-05-25.

**Deviation:** Plan specified `-Wm-yp0x143=0x80` but GBDK emitted a deprecation warning recommending the modern `-Wm-yc` (sets the same byte to 0x80, DMG-compatible). Switched. Also plan said `#include <gb/cpu.h>` for `_cpu` — that header doesn't exist in GBDK-2020; `_cpu` is in `<gb/gb.h>` directly along with `CGB_TYPE = 0x11` named constant which I used instead of the magic number.

**Goal**: Make the cartridge runnable in GBC mode (color hardware exposed) while staying fully playable on DMG. Add the runtime helper `is_gbc()` for later phases to gate color/palette code on. End state: ROM still looks identical on both DMG and GBC (no color changes yet), but the cart header flag and detection plumbing are in place for Phase 2+.

**Why this is its own phase:** The header change is a 1-line Makefile edit but it's the foundation for everything else. Isolating it as its own commit means if anything regresses we can bisect cleanly.

**Branching note:** This work is large enough (~7 phases) that you MAY create a feature branch `v1.1-color-and-chiptune` for the duration. The codebase has been worked on directly on `main` throughout v1.0, but a feature branch lets you iterate without affecting the v1.0 ROM on `main` until you're ready. If you skip the branch and work on `main`, each phase ships as a normal commit. Either choice is fine — just be consistent.

### Task 1.1: Update Makefile to set GBC-aware cart header

**Files:**
- Modify: `Makefile` line 18 (the `LCC_FLAGS` definition)

- [ ] **Step 1: Read the current `LCC_FLAGS` line and its surrounding comments**

Confirm `Makefile:11-18` currently reads:

```makefile
# Compiler / linker flags
# -Wm-yt0x03 → MBC1+RAM+Battery cartridge type byte
# -Wm-yo4   → 4 ROM banks (64KB total)
# -Wm-ya1   → 1 RAM bank (8KB SRAM)
# -Wm-yn"GB" → ROM name in cartridge header
# (No -Wm-yc — that would set CGB-compatible header byte. We target DMG only,
#  so we omit it entirely. The cartridge header's CGB flag stays at 0x00 = DMG.)
LCC_FLAGS := -Wm-yt0x03 -Wm-yo4 -Wm-ya1 -Wm-yn"GB"
```

- [ ] **Step 2: Edit the flags + the comment block to add `-Wm-yp0x143=0x80`**

Replace the block above with:

```makefile
# Compiler / linker flags
# -Wm-yt0x03      → MBC1+RAM+Battery cartridge type byte
# -Wm-yo4         → 4 ROM banks (64KB total)
# -Wm-ya1         → 1 RAM bank (8KB SRAM)
# -Wm-yn"GB"      → ROM name in cartridge header
# -Wm-yp0x143=0x80 → GBC-aware (works on both DMG and GBC). 0x80 means
#                    "supports GBC enhancements but DMG-compatible".
#                    0xC0 would be GBC-only and refuse to boot on DMG;
#                    we don't want that — the v1.0 commitment is DMG support.
LCC_FLAGS := -Wm-yt0x03 -Wm-yo4 -Wm-ya1 -Wm-yn"GB" -Wm-yp0x143=0x80
```

- [ ] **Step 3: Clean rebuild**

```bash
make clean && make 2>&1 | tail -5
```

Expected: clean build, no errors. ROM at `build/gameboygame.gb` is 65536 bytes (unchanged size).

- [ ] **Step 4: Verify cart header byte changed**

```bash
python -c "data = open('build/gameboygame.gb','rb').read(); print(f'CGB flag byte (0x143): 0x{data[0x143]:02X}')"
```

Expected output: `CGB flag byte (0x143): 0x80`

- [ ] **Step 5: Verify ROM still runs on DMG in mGBA**

Open ROM in mGBA. Force DMG mode (Tools → Game Boy → DMG palette: select any DMG palette, or set Settings → Game Boy → Model = "Game Boy (DMG)"). Title screen should render identically to v1.0 — no visual difference, no crash. Walk through TITLE → PLAY → solve one group → see solved bar in greyscale.

- [ ] **Step 6: Verify ROM runs on GBC in mGBA**

Switch mGBA to Game Boy Color mode (Settings → Game Boy → Model = "Game Boy Color"). Reload the ROM. Title screen renders — likely with GBC's default DMG-fallback palette (often a slightly tinted greyscale or one of GBC's hardcoded auto-palettes for unrecognized ROMs). No crash. Walk through to a solved bar — colors may look "off" compared to DMG but the game is playable.

- [ ] **Step 7: Commit**

```bash
git add Makefile
git commit -m "C1.1: flag cart header as GBC-aware (0x143 = 0x80)

Allows the ROM to opt into GBC color hardware on Game Boy Color while
remaining fully playable on DMG. No visual changes yet — palette
setup lands in Phase 2."
```

### Task 1.2: Add `is_gbc()` runtime helper

**Files:**
- Modify: `src/engine/render.h` (add prototype)
- Modify: `src/engine/render.c` (add implementation)

- [ ] **Step 1: Add the prototype to render.h**

Open `src/engine/render.h`. After the existing `#define`s (currently end at line ~36 with `TITLE_BANNER_H`), and before the `void render_init(void);` line, add:

```c
// Returns true if running on a Game Boy Color (or compatible), false on
// original Game Boy (DMG) or Pocket (MGB). All color-code paths in this
// engine check this and no-op on DMG.
//
// Uses GBDK's `_cpu` global: 0x11 = CGB, 0x01 = DMG, 0xFF = MGB.
#include <stdbool.h>
bool is_gbc(void);
```

- [ ] **Step 2: Add the implementation to render.c**

Open `src/engine/render.c`. The current file starts with includes and the static tile-map buffer. After the existing `#include`s at the top, ensure `#include <gb/cpu.h>` is present (likely already included transitively via `<gb/gb.h>`, but make it explicit):

If `<gb/cpu.h>` is not already in the include list, add it after `<gb/gb.h>`:

```c
#include <gb/gb.h>
#include <gb/cpu.h>
```

Then add the function definition. Place it BEFORE `render_init` (so it's available to `render_init` for Phase 2's palette gating). Add immediately after the `static uint8_t dirty = 0;` line:

```c
bool is_gbc(void) {
    return _cpu == 0x11;
}
```

- [ ] **Step 3: Build**

```bash
make 2>&1 | tail -3
```

Expected: clean compile, no warnings about `_cpu`. If you see "undefined `_cpu`", the `<gb/cpu.h>` include is missing — re-check Step 2.

- [ ] **Step 4: Verify by adding a temporary debug render and checking in mGBA**

This is a one-shot diagnostic, NOT a permanent change. The goal: confirm `is_gbc()` returns the right value in both modes.

Temporarily modify `src/main.c` line 40 (`BGP_REG = 0xE4;`) to print the GBC detection result to the title row. Replace lines 39-42:

```c
void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();
```

with (temporary):

```c
void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();
    // TEMPORARY Phase 1.2 verification — remove before commit
    render_text(0, 0, is_gbc() ? "GBC DETECTED" : "DMG DETECTED");
    render_flush();
    for (volatile uint16_t i = 0; i < 50000; i++);  // ~1-second visible delay
```

Build and run in mGBA (DMG mode first, then GBC mode). On the first frame you should see "DMG DETECTED" or "GBC DETECTED" briefly before the TITLE scene draws over it.

- [ ] **Step 5: Revert the temporary diagnostic**

Remove the two added lines from `src/main.c`. Build to confirm clean compile.

```bash
make 2>&1 | tail -3
```

- [ ] **Step 6: Commit**

```bash
git add src/engine/render.h src/engine/render.c
git commit -m "C1.2: add is_gbc() runtime detection helper

Reads GBDK's _cpu register (0x11 = CGB, 0x01 = DMG, 0xFF = MGB).
All color/palette code in Phase 2+ gates on this to no-op on DMG."
```

### Phase 1 group review

After completing Task 1.1 and 1.2:

Review the batch from multiple perspectives. Minimum 3 review rounds:
1. **Build sanity**: clean rebuild succeeds with no new warnings. `make test` still passes (60 tests).
2. **Header verification**: cart header byte 0x143 is 0x80 (use the Python command from 1.1 step 4).
3. **DMG/GBC parity**: ROM looks identical to v1.0 on DMG; ROM doesn't crash on GBC. No color changes yet — that's Phase 2.

If round 3 still finds issues, keep going until clean.

**Phase 1 Execution Status:** when both tasks ship, update the per-phase banner above to `✅ SHIPPED at <SHA> on <date>` and the top-of-plan Execution Status table.

---

## Phase 2 — GBC palette infrastructure

**Execution Status:** ✅ SHIPPED at `42f606d` (palette data) + `ee1733e` (boot load) + `c40428c` (attribute API) on 2026-05-25. User verified all 5 palettes visible in mGBA's GBC palette viewer (initially 6 — palette 5 was later removed in Phase 4 scope cut).

**Goal**: At boot, on GBC, load 5 background palettes (palette 0 = default greyscale, palettes 1-4 = NYT tier colors yellow/green/blue/purple). Add a `render_set_tile_palette(x, y, palette_idx)` helper that writes to VRAM bank 1's attribute map (no-op on DMG). End state: palettes are visible in mGBA's palette viewer (Tools → Game Boy → View palette), but no scene uses them yet so the screen still looks like v1.0.

**Why:** Phase 3 will start coloring tier bars by calling the new helper. Setting up the palettes once at boot keeps scene code simple — scenes only need to know "this tile uses palette N", not the color values themselves.

### Task 2.1: Define palette color data

**Files:**
- Modify: `src/engine/render.h` (add palette index constants)
- Modify: `src/engine/render.c` (add the palette data array)

- [ ] **Step 1: Add palette index constants to render.h**

In `src/engine/render.h`, after the `TITLE_BANNER_H` define and before the `is_gbc()` declaration, add:

```c
// GBC background palette indices. Used as the `palette_idx` argument
// to render_set_tile_palette(). On DMG these constants are ignored.
//
// Palette 0: default greyscale-equivalent (white/light/dark/black).
//   Used for all text, UI chrome, and unspecified tiles.
// Palettes 1-4: NYT tier colors (yellow / green / blue / purple).
//   Used by render_solved_bar in scenes for the colored tier bands.
// Palette 5: title banner accent (Phase 4).
#define GBC_PAL_DEFAULT     0
#define GBC_PAL_TIER_YELLOW 1
#define GBC_PAL_TIER_GREEN  2
#define GBC_PAL_TIER_BLUE   3
#define GBC_PAL_TIER_PURPLE 4
#define GBC_PAL_TITLE       5
```

- [ ] **Step 2: Define palette color data in render.c**

GBDK's `RGB(r, g, b)` macro takes 5-bit channels (0-31) and packs them into the GBC's 16-bit RGB555 format. Each palette is 4 colors (shade 0 = lightest through shade 3 = darkest). Tier bars use UI tiles where the dark pixels (palette index 3) form the label background and light pixels (palette index 0) form the text glyphs (via the inverted font). So:

- Color 0 of a tier palette = light tint (text appears in this color)
- Color 3 of a tier palette = saturated tier color (label background)

In `src/engine/render.c`, after the `static uint8_t dirty = 0;` line and BEFORE the `bool is_gbc(void)` function added in Task 1.2, add:

```c
// GBC background palette data — 6 palettes × 4 colors × 2 bytes per color.
// Format is RGB555 little-endian (GBC native). Each palette's color 0 is
// the "lightest" shade (mapped to font glyph color) and color 3 is the
// "darkest" / most saturated (mapped to label backgrounds, solid fills).
//
// Tier color choices: NYT Connections uses bright, saturated colors that
// are clearly distinguishable. We pick close-to-canonical values that also
// have enough luminance contrast to keep text readable when overlaid.
static const uint16_t gbc_palette_data[6 * 4] = {
    // Palette 0: default greyscale — matches DMG appearance on GBC
    RGB(31, 31, 31), RGB(20, 20, 20), RGB(11, 11, 11), RGB( 0,  0,  0),

    // Palette 1: yellow tier — light cream → golden yellow
    RGB(31, 31, 24), RGB(31, 28, 12), RGB(28, 22,  4), RGB(18, 14,  0),

    // Palette 2: green tier — pale green → leaf green
    RGB(28, 31, 24), RGB(18, 28, 12), RGB( 8, 20,  4), RGB( 2, 12,  0),

    // Palette 3: blue tier — pale sky → deep blue
    RGB(24, 28, 31), RGB(12, 18, 28), RGB( 4,  8, 22), RGB( 0,  2, 14),

    // Palette 4: purple tier — pale lavender → royal purple
    RGB(28, 24, 31), RGB(20, 12, 26), RGB(14,  4, 20), RGB( 8,  0, 12),

    // Palette 5: title banner accent — bright red/orange ("GB" signature color)
    RGB(31, 28, 22), RGB(31, 18,  8), RGB(28,  8,  4), RGB(18,  0,  0),
};
```

- [ ] **Step 3: Build to verify the macro expands and the array compiles**

```bash
make 2>&1 | tail -3
```

Expected: clean build. If `RGB` is undefined, ensure `<gb/gb.h>` is included (it defines `RGB`). If `uint16_t` is unknown, ensure `<stdint.h>` is included (it is — already in render.h).

- [ ] **Step 4: Commit**

```bash
git add src/engine/render.h src/engine/render.c
git commit -m "C2.1: define 6 GBC background palettes (default + 4 tiers + title accent)

RGB555 little-endian, NYT-faithful tier colors. Data only — no
boot-time load yet (that's 2.2) and no scene usage yet (that's
3.1+). Compiles to a static const array — ROM-resident, no SRAM
cost."
```

### Task 2.2: Load palettes at boot on GBC

**Files:**
- Modify: `src/engine/render.c` (the `render_init()` function)

- [ ] **Step 1: Read the current render_init**

`render_init()` currently sets up tile data via `set_bkg_data` calls. Confirm it's near the top of render.c. It should look like:

```c
void render_init(void) {
    set_bkg_data(0, font_TILE_COUNT, font_tiles);
    set_bkg_data(UI_TILE_BASE, ui_tiles_TILE_COUNT, ui_tiles_tiles);
    set_bkg_data(FONT_INV_TILE_BASE, font_inv_TILE_COUNT, font_inv_tiles);
    set_bkg_data(TITLE_TILE_BASE, title_TILE_COUNT, title_tiles);
    render_clear();
}
```

- [ ] **Step 2: Add palette loading at the end of render_init**

GBDK provides `set_bkg_palette(first_palette, num_palettes, data_ptr)` which writes to GBC palette RAM via BCPS/BCPD. It's a no-op on DMG (the GBDK function checks `_cpu` internally). We call it unconditionally for cleanliness.

Modify `render_init` to add the palette load AFTER the existing tile setup but BEFORE `render_clear()`:

```c
void render_init(void) {
    set_bkg_data(0, font_TILE_COUNT, font_tiles);
    set_bkg_data(UI_TILE_BASE, ui_tiles_TILE_COUNT, ui_tiles_tiles);
    set_bkg_data(FONT_INV_TILE_BASE, font_inv_TILE_COUNT, font_inv_tiles);
    set_bkg_data(TITLE_TILE_BASE, title_TILE_COUNT, title_tiles);

    // GBC: load all 6 background palettes. set_bkg_palette is a no-op on
    // DMG (checks _cpu internally), so this is safe to call unconditionally.
    set_bkg_palette(0, 6, gbc_palette_data);

    render_clear();
}
```

- [ ] **Step 3: Build**

```bash
make 2>&1 | tail -3
```

Expected: clean build. If `set_bkg_palette` is undefined, it's in `<gb/gb.h>` which is already included.

- [ ] **Step 4: Verify in mGBA palette viewer**

Open the ROM in mGBA, set model to "Game Boy Color", launch. Open Tools → Game Boy → View palette (or "Background palette" depending on mGBA version). You should see 6 palettes loaded:

- Palette 0: greyscale gradient
- Palette 1: yellow gradient
- Palette 2: green gradient
- Palette 3: blue gradient
- Palette 4: purple gradient
- Palette 5: red/orange gradient

Palettes 6-7 will be whatever the GBC default is (probably greyscale-ish or zeros — doesn't matter, we won't use them).

If the palettes appear correctly, screen still looks like v1.0 (because no tile uses anything other than palette 0 yet — the default attribute byte is 0).

- [ ] **Step 5: Verify on DMG (no regression)**

Switch mGBA to DMG mode, reload. Screen looks identical to v1.0. Palette viewer is unavailable on DMG (or shows empty).

- [ ] **Step 6: Commit**

```bash
git add src/engine/render.c
git commit -m "C2.2: load all 6 GBC palettes at boot

set_bkg_palette is a no-op on DMG so this stays DMG-safe. Palettes are
now visible in mGBA's GBC palette viewer; no tile attribute writes yet
so the screen still looks identical to v1.0."
```

### Task 2.3: Add `render_set_tile_palette` API + attribute writes to render_flush

**Files:**
- Modify: `src/engine/render.h` (add the new function prototype)
- Modify: `src/engine/render.c` (add a parallel attribute buffer + implementation)

**Architectural note:** The existing render pipeline uses a CPU-side `tilemap_buf` and `render_flush()` pushes the whole buffer in one `set_bkg_tiles` call during VBlank. For per-tile palette assignment we need a parallel `attr_buf` containing palette indices, and `render_flush` must additionally push it to VRAM bank 1 (GBC only — DMG ignores VBK_REG entirely so the write goes nowhere harmlessly, but we still gate to avoid the wasted cycles).

- [ ] **Step 1: Add prototype to render.h**

In `src/engine/render.h`, add this prototype after `render_set_tile`:

```c
// Set the GBC background palette index for a single tile. On DMG this
// is a no-op (DMG has no per-tile palettes). On GBC, color rendering of
// the tile at (x, y) uses palette `palette_idx` (one of the GBC_PAL_*
// constants above). Must be paired with a corresponding render_set_tile
// or render_text* call to actually write tile data at that cell —
// palette assignment alone doesn't draw anything.
void render_set_tile_palette(uint8_t x, uint8_t y, uint8_t palette_idx);
```

- [ ] **Step 2: Add the parallel attribute buffer + implementation to render.c**

In `src/engine/render.c`, just after the existing `static uint8_t tilemap_buf[...];` declaration:

```c
// Parallel attribute buffer for GBC tile-attribute writes (palette index
// in low 3 bits; bits 3-7 unused — we don't use VRAM bank 1 for tiles,
// X/Y flip, or priority). Maintained alongside tilemap_buf and pushed
// to VRAM bank 1 in render_flush. On DMG this buffer is harmlessly
// maintained but not pushed to VRAM (DMG has no bank 1).
static uint8_t attr_buf[SCREEN_TILES_W * SCREEN_TILES_H];
```

Then add the function implementation. Place it after `render_set_tile`:

```c
void render_set_tile_palette(uint8_t x, uint8_t y, uint8_t palette_idx) {
    if (x >= SCREEN_TILES_W || y >= SCREEN_TILES_H) return;
    attr_buf[y * SCREEN_TILES_W + x] = palette_idx & 0x07;
    dirty = 1;
}
```

- [ ] **Step 3: Modify render_flush to push attr_buf to VRAM bank 1 on GBC**

Replace the existing `render_flush` function:

```c
void render_flush(void) {
    if (!dirty) return;
    set_bkg_tiles(0, 0, SCREEN_TILES_W, SCREEN_TILES_H, tilemap_buf);
    if (is_gbc()) {
        // Switch to VRAM bank 1 to write tile-attribute data, then back
        // to bank 0 (which holds tile graphics and the regular tilemap).
        // GBDK's set_bkg_tiles writes to whichever bank is currently
        // selected, so the bank switch is essential.
        VBK_REG = 1;
        set_bkg_tiles(0, 0, SCREEN_TILES_W, SCREEN_TILES_H, attr_buf);
        VBK_REG = 0;
    }
    dirty = 0;
}
```

- [ ] **Step 4: Also clear attr_buf in render_clear**

`render_clear()` currently only zeros `tilemap_buf`. It should also zero `attr_buf` so a cleared scene starts with palette 0 (default greyscale) everywhere:

```c
void render_clear(void) {
    memset(tilemap_buf, 0, sizeof(tilemap_buf));
    memset(attr_buf, 0, sizeof(attr_buf));
    dirty = 1;
}
```

- [ ] **Step 5: Build**

```bash
make 2>&1 | tail -3
```

Expected: clean build. If `VBK_REG` is undefined, it's in `<gb/hardware.h>` (which is transitively included via `<gb/gb.h>`).

- [ ] **Step 6: Verify in mGBA (DMG + GBC, no visual change yet)**

Both DMG and GBC modes: screen looks identical to v1.0. Phase 2 is plumbing only — no scene calls `render_set_tile_palette` yet. The attribute buffer is all zeros on every frame, so every tile uses palette 0 (the default greyscale).

In mGBA's GBC palette viewer, palettes 1-5 are loaded (per Task 2.2) but unused.

- [ ] **Step 7: Commit**

```bash
git add src/engine/render.h src/engine/render.c
git commit -m "C2.3: parallel attribute buffer + render_set_tile_palette API

attr_buf mirrors tilemap_buf layout. render_flush pushes it to VRAM
bank 1 on GBC (gated by is_gbc); DMG path unchanged. render_clear
zeros both buffers. No scenes use the new API yet — Phase 3 adds tier
bar palette writes."
```

### Phase 2 group review

After completing Tasks 2.1, 2.2, 2.3:

Review the batch from multiple perspectives. Minimum 3 review rounds:
1. **DMG no-regression**: Screen is pixel-identical to v1.0 on DMG (run in mGBA DMG mode, walk through TITLE → PLAY → solve a group → check WIN screen).
2. **GBC palette load**: mGBA palette viewer shows palettes 0-5 with the expected RGB values from Task 2.1. Palette 0 is greyscale; palettes 1-4 are tier colors; palette 5 is red/orange accent.
3. **GBC no-regression**: Screen is identical to v1.0 on GBC (since no scene uses non-zero palette indices yet). All 60 host-side tests still pass (`make test`).

If round 3 still finds issues, keep going until clean.

---

## Phase 3 — Apply tier colors to gameplay scenes

**Execution Status:** ✅ SHIPPED at `7f847f2` (scene_play) + `772b4e8` (scene_win) + `4314638` (scene_lose) on 2026-05-25. User verified all 4 tiers render in NYT yellow/green/blue/purple on GBC across all 3 scenes; DMG unchanged from v1.0.

**Goal**: Tier bars in scene_play, scene_win, and scene_lose render in their tier color on GBC (yellow/green/blue/purple). DMG rendering unchanged.

**Why:** This is the user-visible payoff for Phases 1-2. The four tier colors are conceptually about COLOR (they're called "yellow tier", "green tier", etc.); rendering them as different shading patterns on the DMG was the v1.0 compromise. On GBC they finally render as the colors they represent.

**Approach:** Each scene has a `render_solved_bar(puzzle, tier, y)` or `render_bar(puzzle, tier, y)` helper that paints the 20 tiles of the bar row. After the existing tile writes, add a loop that paints the same 20 tiles with `render_set_tile_palette(x, y, palette_for_tier)` where `palette_for_tier = GBC_PAL_TIER_YELLOW + tier` (since the tier enum is 0=yellow, 1=green, 2=blue, 3=purple — matches palette ordering).

### Task 3.1: Color tier bars in scene_play

**Files:**
- Modify: `src/game/scene_play.c` (the `render_solved_bar` function at lines 113-128)

- [ ] **Step 1: Read the current render_solved_bar**

Confirm the current implementation in `scene_play.c:113-128` looks like:

```c
static void render_solved_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y) {
    // Bar layout: [3 tier-pattern tiles] [14 solid-dark label tiles] [3 tier-pattern]
    // Category name overlays the label using the inverted font tiles
    // (light glyphs on dark — Plan C addition).
    uint8_t pattern_tile = (uint8_t)(UI_TILE_PATTERN_BASE + tier);
    for (uint8_t x = 0; x < 3; x++)   render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 17; x < 20; x++) render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 3; x < 17; x++)  render_set_tile(x, y, UI_TILE_SOLID_DARK);

    // Overlay category name centered in the 14-tile label region (cols 3..16)
    const char *name = puzzle->category_names[tier];
    uint8_t name_len = 0;
    while (name[name_len] && name_len < 14) name_len++;
    uint8_t text_x = (uint8_t)(3 + (14 - name_len) / 2);
    render_text_inv(text_x, y, name);
}
```

- [ ] **Step 2: Append palette writes at the end of the function**

After `render_text_inv(text_x, y, name);` but before the closing `}`, add a loop that paints palette indices for all 20 tiles of the bar row:

```c
    // Phase 3 (Plan D): paint the GBC palette index for the whole 20-tile
    // bar row so the tile pattern + solid + text glyphs render in the tier
    // color. tier enum (0=yellow..3=purple) maps directly to palette
    // indices (GBC_PAL_TIER_YELLOW..GBC_PAL_TIER_PURPLE = 1..4).
    // No-op on DMG (render_set_tile_palette is a no-op there).
    uint8_t palette = (uint8_t)(GBC_PAL_TIER_YELLOW + tier);
    for (uint8_t x = 0; x < SCREEN_TILES_W; x++) {
        render_set_tile_palette(x, y, palette);
    }
```

The full updated function should be:

```c
static void render_solved_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y) {
    // Bar layout: [3 tier-pattern tiles] [14 solid-dark label tiles] [3 tier-pattern]
    // Category name overlays the label using the inverted font tiles
    // (light glyphs on dark — Plan C addition).
    uint8_t pattern_tile = (uint8_t)(UI_TILE_PATTERN_BASE + tier);
    for (uint8_t x = 0; x < 3; x++)   render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 17; x < 20; x++) render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 3; x < 17; x++)  render_set_tile(x, y, UI_TILE_SOLID_DARK);

    // Overlay category name centered in the 14-tile label region (cols 3..16)
    const char *name = puzzle->category_names[tier];
    uint8_t name_len = 0;
    while (name[name_len] && name_len < 14) name_len++;
    uint8_t text_x = (uint8_t)(3 + (14 - name_len) / 2);
    render_text_inv(text_x, y, name);

    // Phase 3 (Plan D): paint the GBC palette index for the whole 20-tile
    // bar row so the tile pattern + solid + text glyphs render in the tier
    // color. tier enum (0=yellow..3=purple) maps directly to palette
    // indices (GBC_PAL_TIER_YELLOW..GBC_PAL_TIER_PURPLE = 1..4).
    // No-op on DMG (render_set_tile_palette is a no-op there).
    uint8_t palette = (uint8_t)(GBC_PAL_TIER_YELLOW + tier);
    for (uint8_t x = 0; x < SCREEN_TILES_W; x++) {
        render_set_tile_palette(x, y, palette);
    }
}
```

- [ ] **Step 3: Build**

```bash
make 2>&1 | tail -3
```

Expected: clean build. If `GBC_PAL_TIER_YELLOW` is undefined, the new render.h constants from Task 2.1 didn't propagate — re-check.

- [ ] **Step 4: Verify in mGBA (GBC mode)**

Open ROM in mGBA, GBC mode. Start a NEW GAME, solve any group of 4 (use the first puzzle: BIRDS = ROBIN, EAGLE, HAWK, FINCH). Submit with START.

Expected: the solved bar appears at the top of the screen in the tier color matching the solved group (yellow/green/blue/purple). The category name overlays the bar in a light shade against the dark tier color. Subsequent solves stack additional colored bars.

- [ ] **Step 5: Verify in mGBA (DMG mode)**

Switch mGBA to DMG, reload, solve a group. Expected: solved bar renders exactly as in v1.0 — greyscale with the hatched tier pattern at the ends and dark solid in the middle. No regression.

- [ ] **Step 6: Commit**

```bash
git add src/game/scene_play.c
git commit -m "C3.1: scene_play tier bars use GBC palette colors

render_solved_bar now also writes palette indices for the 20-tile bar
row. On GBC: yellow/green/blue/purple bars match their tier name. On
DMG: unchanged from v1.0."
```

### Task 3.2: Color tier bars in scene_win

**Files:**
- Modify: `src/game/scene_win.c` (the `render_bar` function at lines 26-39)

- [ ] **Step 1: Read the current render_bar**

In `scene_win.c:26-39`:

```c
static void render_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y) {
    // Same shape as scene_play's solved bar (Plan C now overlays the
    // category name via render_text_inv).
    uint8_t pattern_tile = (uint8_t)(UI_TILE_PATTERN_BASE + tier);
    for (uint8_t x = 0; x < 3; x++)   render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 17; x < 20; x++) render_set_tile(x, y, pattern_tile);
    for (uint8_t x = 3; x < 17; x++)  render_set_tile(x, y, UI_TILE_SOLID_DARK);

    const char *name = puzzle->category_names[tier];
    uint8_t name_len = 0;
    while (name[name_len] && name_len < 14) name_len++;
    uint8_t text_x = (uint8_t)(3 + (14 - name_len) / 2);
    render_text_inv(text_x, y, name);
}
```

- [ ] **Step 2: Add palette writes — same pattern as 3.1**

Append before the closing `}`:

```c

    uint8_t palette = (uint8_t)(GBC_PAL_TIER_YELLOW + tier);
    for (uint8_t x = 0; x < SCREEN_TILES_W; x++) {
        render_set_tile_palette(x, y, palette);
    }
```

- [ ] **Step 3: Build**

```bash
make 2>&1 | tail -3
```

- [ ] **Step 4: Verify in mGBA (GBC)**

Solve a full puzzle (4 groups) → transition to WIN scene. Watch the cascade: 4 bars appear in yellow / green / blue / purple sequentially. Stats text below uses the default greyscale palette (no palette write touches those tiles).

- [ ] **Step 5: Verify on DMG**

Same flow on DMG mode — cascade bars in greyscale as v1.0.

- [ ] **Step 6: Commit**

```bash
git add src/game/scene_win.c
git commit -m "C3.2: scene_win tier bars use GBC palette colors"
```

### Task 3.3: Color tier bars in scene_lose

**Files:**
- Modify: `src/game/scene_lose.c` (find the bar-rendering helper)

- [ ] **Step 1: Find the bar helper in scene_lose.c**

Open `src/game/scene_lose.c`. Find the function that renders the colored tier bars (likely named `render_bar` or `render_solved_bar`, similar shape to scene_win's). It will have the same 3-pattern + 14-solid + 3-pattern structure.

- [ ] **Step 2: Add palette writes at the end of that function**

Add the same 5-line palette loop as Task 3.1 / 3.2 just before the closing `}`:

```c

    uint8_t palette = (uint8_t)(GBC_PAL_TIER_YELLOW + tier);
    for (uint8_t x = 0; x < SCREEN_TILES_W; x++) {
        render_set_tile_palette(x, y, palette);
    }
```

If scene_lose has the same `tier` parameter and `y` parameter naming as scene_win, the line is verbatim. If it differs, adjust variable names to match the surrounding code.

- [ ] **Step 3: Build**

```bash
make 2>&1 | tail -3
```

- [ ] **Step 4: Verify in mGBA (GBC)**

Trigger a LOSE: start a puzzle, intentionally submit 4 wrong groups in a row. LOSE scene appears showing the correct answers in colored tier bars.

- [ ] **Step 5: Verify on DMG**

Same flow on DMG — bars in greyscale as v1.0.

- [ ] **Step 6: Commit**

```bash
git add src/game/scene_lose.c
git commit -m "C3.3: scene_lose tier bars use GBC palette colors"
```

### Phase 3 group review

After completing Tasks 3.1, 3.2, 3.3:

Review the batch from multiple perspectives. Minimum 3 review rounds:
1. **GBC visual correctness**: solved bars on PLAY are colored. WIN cascade is all 4 tiers in distinct colors. LOSE shows all 4 colored answer bars. Yellow/green/blue/purple are immediately distinguishable; category name text is readable against each tier background.
2. **DMG no-regression**: same scenes on DMG mode show greyscale identical to v1.0.
3. **No flicker or attribute-write race**: the play scene's incremental rendering (A-press → single cell repaint) still works without visible flicker. Attribute writes go through the same render_flush mechanism — verify A-press still flicker-free.

If round 3 still finds issues, keep going until clean.

---

## Phase 4 — Title banner accent color

**Execution Status:** ✅ SHIPPED (as a scope cut) on 2026-05-25.

**Deviation from plan:** Initially implemented per spec — applied palette 5 (originally bright red/orange, then user-iterated to warm peach/coral) to the title banner via `render_set_tile_palette(GBC_PAL_TITLE)`. User preferred the unstylized greyscale title from v1.0/Plan C: "I kind of liked the old black and white." Decision: revert all title-color work. Title banner stays on palette 0 (default greyscale) on both DMG and GBC. The in-game tier colors (Phase 3) are the sole visual signature on GBC.

**What was removed:**
- The two doubly-nested loops in `scene_title.c`'s banner render block that wrote `GBC_PAL_TITLE` over the 8×4 banner region.
- Palette 5 (warm peach/coral) data in `src/engine/render.c`'s `gbc_palette_data[]` — array shrank from 6×4 to 5×4 entries.
- `GBC_PAL_TITLE` constant in `src/engine/render.h`.
- The `set_bkg_palette(0, 6, …)` call became `set_bkg_palette(0, 5, …)`.

**Rationale (saved as a discovery):** "Less is more" — adding color to chrome that already works in monochrome can dilute the impact of the color elements that genuinely benefit from it. The tier bars NEED color (4 colors with meaning); the title doesn't (the banner is decoration). Future polish work should treat color as a feature for differentiation, not as a decoration.

**Goal**: On GBC, the "GB" banner on the title screen renders in the accent color (palette 5 — bright red/orange) instead of greyscale. DMG unchanged.

**Why:** The title screen is the player's first impression. With music landing in Phase 5-6, the accent color completes the "this is a polished color-aware game" first-frame experience.

### Task 4.1: Apply title accent palette to the banner

**Files:**
- Modify: `src/game/scene_title.c` (the banner rendering block at lines ~65-76)

- [ ] **Step 1: Read the current banner rendering**

In `scene_title.c` the banner renders via:

```c
{
    uint8_t banner_x = (uint8_t)((SCREEN_TILES_W - TITLE_BANNER_W) / 2);
    uint8_t banner_y = 1;
    for (uint8_t r = 0; r < TITLE_BANNER_H; r++) {
        for (uint8_t c = 0; c < TITLE_BANNER_W; c++) {
            uint8_t tile_idx = (uint8_t)(r * TITLE_BANNER_W + c);
            render_set_tile((uint8_t)(banner_x + c),
                            (uint8_t)(banner_y + r),
                            (uint8_t)(TITLE_TILE_BASE + tile_idx));
        }
    }
}
```

This paints an 8-wide × 4-tall block of tiles starting at column 6, row 1.

- [ ] **Step 2: Add a parallel loop that writes the GBC_PAL_TITLE attribute**

Inside the same `{ }` block, immediately after the closing brace of the inner `for (uint8_t c …)` loop but still inside the outer `for (uint8_t r …)` loop... actually simpler: add a second separate doubly-nested loop right after the existing one. Place it before the closing `}` of the outer `{ }` block:

```c
{
    uint8_t banner_x = (uint8_t)((SCREEN_TILES_W - TITLE_BANNER_W) / 2);
    uint8_t banner_y = 1;
    for (uint8_t r = 0; r < TITLE_BANNER_H; r++) {
        for (uint8_t c = 0; c < TITLE_BANNER_W; c++) {
            uint8_t tile_idx = (uint8_t)(r * TITLE_BANNER_W + c);
            render_set_tile((uint8_t)(banner_x + c),
                            (uint8_t)(banner_y + r),
                            (uint8_t)(TITLE_TILE_BASE + tile_idx));
        }
    }
    // Phase 4 (Plan D): paint the GBC accent palette over the banner's
    // 8×4 tile region. No-op on DMG.
    for (uint8_t r = 0; r < TITLE_BANNER_H; r++) {
        for (uint8_t c = 0; c < TITLE_BANNER_W; c++) {
            render_set_tile_palette((uint8_t)(banner_x + c),
                                    (uint8_t)(banner_y + r),
                                    GBC_PAL_TITLE);
        }
    }
}
```

- [ ] **Step 3: Build**

```bash
make 2>&1 | tail -3
```

- [ ] **Step 4: Verify in mGBA (GBC)**

Open ROM in GBC mode. Title screen: "GB" banner appears in red/orange tones; everything else (CONNECTIONS subtitle, menu, stats, attribution) still in default greyscale.

- [ ] **Step 5: Verify on DMG**

Title screen looks like v1.0 in greyscale.

- [ ] **Step 6: Commit**

```bash
git add src/game/scene_title.c
git commit -m "C4: title banner uses GBC accent palette (red/orange)

DMG: unchanged (default greyscale). GBC: 'GB' banner renders in the
palette 5 accent color, giving the title screen visual identity."
```

### Phase 4 group review

After Task 4.1:
1. **Accent color reads as intended**: red/orange is distinct from any tier color (yellow/green/blue/purple), making the banner immediately recognizable as title chrome rather than a tier.
2. **DMG no-regression**: title screen identical to v1.0.
3. **No bleed into menu region**: only the 8×4 banner tiles (rows 1-4, cols 6-13) get the accent palette. The CONNECTIONS subtitle on row 6, menu items, and attribution row are still palette 0 (greyscale).

---

## Phase 5 — Music player engine

**Execution Status:** ⬜ NOT STARTED

**Goal**: New `src/engine/music.{h,c}` module. Pattern-based player: define a `MusicTrack` as an array of `{note_index, duration_frames}`. `music_tick()` called per VBlank advances the playhead and writes NR13/NR14 when a new note starts. Includes host-side tests of the state machine.

**Why:** Audio polish for v1.1. CH1 reserved for music; CH2/CH4 stay for SFX. The engine is content-agnostic — Phase 6 supplies the actual title theme data.

**Approach:** Reuse the note-frequency constants pattern from `sound.c` (NOTE_C4_LO/HI through NOTE_C6_LO/HI). Music notes are array indices into a shared note table. Special "rest" note value (0xFF) silences CH1 for the duration. Track has a loop point: when playhead reaches `track->length`, it jumps back to `track->loop_start`.

**Mandatory TDD:** the state machine (advancement, looping, rest handling) IS host-testable. Hardware register writes are NOT (we mock those out in tests via a separate `music_emit_note(note_idx)` function that has a host-side stub).

### Task 5.1: Define music.h API + types

**Files:**
- Create: `src/engine/music.h`

- [ ] **Step 1: Write the header**

```c
#ifndef ENGINE_MUSIC_H
#define ENGINE_MUSIC_H

#include <stdint.h>
#include <stdbool.h>

// Music note indices. 0..N-1 reference a shared frequency table in
// music.c; 0xFF is a special "rest" marker that silences CH1.
//
// We support a small set of notes covering 2 octaves (C4..C6) — enough
// for simple chiptune melodies without bloating the frequency table.
// Each note maps to a (NR13, NR14) pair in music_note_table[] (music.c).
#define MUSIC_NOTE_C4   0
#define MUSIC_NOTE_D4   1
#define MUSIC_NOTE_E4   2
#define MUSIC_NOTE_F4   3
#define MUSIC_NOTE_G4   4
#define MUSIC_NOTE_A4   5
#define MUSIC_NOTE_B4   6
#define MUSIC_NOTE_C5   7
#define MUSIC_NOTE_D5   8
#define MUSIC_NOTE_E5   9
#define MUSIC_NOTE_F5  10
#define MUSIC_NOTE_G5  11
#define MUSIC_NOTE_A5  12
#define MUSIC_NOTE_B5  13
#define MUSIC_NOTE_C6  14
#define MUSIC_NOTE_REST 0xFF

#define MUSIC_NOTE_COUNT 15

// One step of a track: play `note` for `duration_frames` frames (60 Hz),
// then advance to the next step. note == MUSIC_NOTE_REST silences CH1
// for the duration.
typedef struct {
    uint8_t  note;
    uint8_t  duration_frames;
} MusicStep;

// A complete track: array of steps with a loop point. Playhead jumps
// from step `length - 1` back to step `loop_start` after that step's
// duration elapses. For non-looping tracks set loop_start == length
// (player will stop after the last step).
typedef struct {
    const MusicStep *steps;
    uint8_t          length;
    uint8_t          loop_start;
} MusicTrack;

// Initialize the music subsystem. Currently a no-op — provided for
// API symmetry with sound_init / render_init. Safe to call multiple
// times.
void music_init(void);

// Start playing `track`. Resets playhead to step 0. If a track was
// already playing, this overrides it. Pass NULL to stop (or use
// music_stop()).
void music_play(const MusicTrack *track);

// Stop the current track and silence CH1. Idempotent.
void music_stop(void);

// Per-frame tick. MUST be called from the VBlank ISR exactly once per
// frame (60 Hz). Advances the playhead and writes NR13/NR14 if a new
// note starts on this frame.
void music_tick(void);

// Returns true if a track is currently playing (i.e., not stopped /
// not at end of non-looping track).
bool music_is_playing(void);

// --- Testability hooks ---
//
// Host-side tests can query these to verify state machine behavior
// without needing to mock GB hardware registers. NOT for production
// use from scenes.

// Index of the step currently playing (0..track->length - 1), or 0xFF
// if no track is playing.
uint8_t music_current_step(void);

// Frames remaining in the current step before advancement. Useful for
// tests of looping and step-boundary edge cases.
uint8_t music_step_frames_remaining(void);

#endif
```

- [ ] **Step 2: Verify the header compiles by including it from main.c (temporary)**

In `src/main.c`, add `#include "engine/music.h"` at the top alongside other engine includes. Build:

```bash
make 2>&1 | tail -3
```

Expected: clean preprocess (will fail to LINK because no music.c yet — that's fine, we want to confirm the header itself is syntactically valid).

If you get linker errors about missing `music_init`, `music_play`, etc., that means the header is correct and the linker is just missing implementations. Move on. If you get COMPILE errors (preprocessor / syntax) inside music.h, fix those.

Remove the `#include "engine/music.h"` line from main.c again — we'll add it back in Task 6.2 when scene_title actually uses it.

- [ ] **Step 3: Commit**

```bash
git add src/engine/music.h src/main.c
# (main.c should be unchanged after Step 2 cleanup, but add it just in case)
git commit -m "C5.1: music.h API — track/step types, play/stop/tick interface

Pattern-based player: MusicTrack = array of {note, duration_frames}
with a loop point. CH1-only. Includes test-only hooks
(music_current_step / music_step_frames_remaining) for the host-side
state-machine tests landing in 5.3."
```

### Task 5.2: Implement music.c — state machine + note table

**Files:**
- Create: `src/engine/music.c`

- [ ] **Step 1: Write the implementation**

```c
#include "music.h"
#include <gb/gb.h>
#include <gb/hardware.h>
#include <stdint.h>

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

// Write NR10-NR14 to play the note (or silence on REST). Volume
// envelope: NR12 = 0x83 → volume 8, decay step 3 — gentle decay so
// successive notes don't bleed into each other.
static void emit_note(uint8_t note) {
    if (note == MUSIC_NOTE_REST) {
        // Silence CH1 by setting envelope to volume 0.
        NR12_REG = 0x00;
        // Trigger with the silence envelope so the channel stops emitting.
        NR14_REG = 0x80;
        return;
    }
    if (note >= MUSIC_NOTE_COUNT) return;  // defensive
    NR10_REG = 0x00;        // no frequency sweep
    NR11_REG = 0x80;        // 50% duty cycle, length unused
    NR12_REG = 0x83;        // volume 8, decay, envelope step 3
    NR13_REG = music_note_table[note].lo;
    NR14_REG = music_note_table[note].hi;
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
}

void music_stop(void) {
    active_track = 0;
    current_step_idx = 0xFF;
    frames_remaining = 0;
    emit_note(MUSIC_NOTE_REST);
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
```

- [ ] **Step 2: Add music.c to the SRC list in Makefile**

Open `Makefile`. Find the `SRC :=` block at lines 53-61. Add `src/engine/music.c` to the engine sources line:

```makefile
SRC := src/main.c \
       src/engine/render.c src/engine/input.c src/engine/save.c \
       src/engine/sound.c src/engine/anim.c src/engine/music.c \
       src/game/puzzle_logic.c src/game/layout.c \
       src/game/scene_title.c src/game/scene_play.c src/game/scene_win.c \
       src/game/scene_lose.c src/game/scene_all_done.c
```

- [ ] **Step 3: Build**

```bash
make 2>&1 | tail -5
```

Expected: clean compile of music.c, clean link.

- [ ] **Step 4: Commit**

```bash
git add src/engine/music.c Makefile
git commit -m "C5.2: implement music.c — state machine + CH1 emission

Pattern player advances per VBlank tick. Note table covers C4..C6 (15
notes) reusing sound.c's frequency-byte format. emit_note writes
NR10-NR14; MUSIC_NOTE_REST sets volume 0 to silence the channel.
Loop point support via track->loop_start."
```

### Task 5.3: Host-side tests for music state machine

**Files:**
- Create: `test/test_music.c`
- Modify: `test/Makefile` (add the new test binary)
- Modify: `src/engine/music.c` — guard the GBDK header for host testability

**Important architectural note:** music.c uses `gb/gb.h` and `gb/hardware.h` for the NR1x register macros. These headers don't compile on host (gcc) — they're GBDK-specific. For host testing, we conditionally compile-out the `emit_note` function body (state machine logic stays intact; only the register writes vanish). The state machine is what we want to test, so this is the right boundary.

- [ ] **Step 1: Make `emit_note` host-safe in music.c**

Wrap the body of `emit_note` (NOT the function signature) in a GBDK guard. Open `src/engine/music.c` and find the `static void emit_note(uint8_t note)` function. Replace it with:

```c
static void emit_note(uint8_t note) {
#ifdef __SDCC
    if (note == MUSIC_NOTE_REST) {
        NR12_REG = 0x00;
        NR14_REG = 0x80;
        return;
    }
    if (note >= MUSIC_NOTE_COUNT) return;
    NR10_REG = 0x00;
    NR11_REG = 0x80;
    NR12_REG = 0x83;
    NR13_REG = music_note_table[note].lo;
    NR14_REG = music_note_table[note].hi;
#else
    (void)note;  // suppress unused-parameter warning on host
#endif
}
```

`__SDCC` is defined automatically by the SDCC compiler (used for the actual GB build). On host gcc it's not defined, so the register-write code is excluded.

Also wrap the GB header includes:

```c
#include "music.h"
#include <stdint.h>
#ifdef __SDCC
#include <gb/gb.h>
#include <gb/hardware.h>
#endif
```

- [ ] **Step 2: Write the failing test file**

Create `test/test_music.c`:

```c
// Host-side tests for the music state machine (src/engine/music.c).
// Builds with mingw gcc — uses no GBDK headers (those are #ifdef __SDCC
// guarded in music.c for this purpose). Tests the state machine's frame
// counting, step advancement, and loop behavior; does NOT test the
// emit_note hardware writes (untestable without a GB emulator).

#include "../src/engine/music.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT(cond) do { \
    tests_run++; \
    if (!(cond)) { \
        tests_failed++; \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

// ----- Test 1: empty player state after init -----
static void test_init_no_active_track(void) {
    music_init();
    ASSERT(music_is_playing() == 0);
    ASSERT(music_current_step() == 0xFF);
}

// ----- Test 2: music_play with NULL stops -----
static void test_play_null_stops(void) {
    music_init();
    music_play(0);
    ASSERT(music_is_playing() == 0);
}

// ----- Test 3: music_play with zero-length track stops -----
static void test_play_empty_track_stops(void) {
    music_init();
    MusicTrack t = { .steps = 0, .length = 0, .loop_start = 0 };
    music_play(&t);
    ASSERT(music_is_playing() == 0);
}

// ----- Test 4: single-step track advances frame counter then stops -----
static void test_single_step_no_loop_stops(void) {
    music_init();
    MusicStep steps[] = { { MUSIC_NOTE_C5, 3 } };
    MusicTrack t = { .steps = steps, .length = 1, .loop_start = 1 };  // no loop
    music_play(&t);
    ASSERT(music_is_playing() == 1);
    ASSERT(music_current_step() == 0);
    ASSERT(music_step_frames_remaining() == 3);

    music_tick();  // frames_remaining: 3 -> 2
    ASSERT(music_step_frames_remaining() == 2);

    music_tick();  // 2 -> 1
    ASSERT(music_step_frames_remaining() == 1);

    music_tick();  // 1 -> 0
    ASSERT(music_step_frames_remaining() == 0);

    music_tick();  // step ends, next_idx == length, loop_start >= length → stop
    ASSERT(music_is_playing() == 0);
}

// ----- Test 5: multi-step track advances through steps -----
static void test_multi_step_advancement(void) {
    music_init();
    MusicStep steps[] = {
        { MUSIC_NOTE_C5, 2 },
        { MUSIC_NOTE_E5, 2 },
        { MUSIC_NOTE_G5, 2 },
    };
    MusicTrack t = { .steps = steps, .length = 3, .loop_start = 3 };  // no loop
    music_play(&t);

    ASSERT(music_current_step() == 0);
    music_tick(); music_tick();  // step 0 done
    music_tick();  // advances to step 1
    ASSERT(music_current_step() == 1);
    ASSERT(music_step_frames_remaining() == 2);
    music_tick(); music_tick();  // step 1 done
    music_tick();  // advances to step 2
    ASSERT(music_current_step() == 2);
    music_tick(); music_tick();
    music_tick();  // end of track, no loop
    ASSERT(music_is_playing() == 0);
}

// ----- Test 6: looping track jumps back to loop_start -----
static void test_loop_back(void) {
    music_init();
    MusicStep steps[] = {
        { MUSIC_NOTE_C5, 1 },  // step 0 — only played once (intro)
        { MUSIC_NOTE_D5, 1 },  // step 1 — looped
        { MUSIC_NOTE_E5, 1 },  // step 2 — looped
    };
    MusicTrack t = { .steps = steps, .length = 3, .loop_start = 1 };
    music_play(&t);

    ASSERT(music_current_step() == 0);
    music_tick();  // step 0 done, advances to step 1
    ASSERT(music_current_step() == 1);
    music_tick();  // step 1 done, advances to step 2
    ASSERT(music_current_step() == 2);
    music_tick();  // step 2 done, loop back to loop_start = 1
    ASSERT(music_current_step() == 1);
    ASSERT(music_is_playing() == 1);  // still playing after loop
    music_tick();  // step 1 done again, advances to step 2
    ASSERT(music_current_step() == 2);
}

// ----- Test 7: music_stop while playing silences the channel -----
static void test_stop_while_playing(void) {
    music_init();
    MusicStep steps[] = { { MUSIC_NOTE_C5, 10 } };
    MusicTrack t = { .steps = steps, .length = 1, .loop_start = 1 };
    music_play(&t);
    ASSERT(music_is_playing() == 1);
    music_stop();
    ASSERT(music_is_playing() == 0);
    ASSERT(music_current_step() == 0xFF);
}

// ----- Test 8: REST note advances normally -----
static void test_rest_note_advances(void) {
    music_init();
    MusicStep steps[] = {
        { MUSIC_NOTE_C5,  2 },
        { MUSIC_NOTE_REST, 2 },
        { MUSIC_NOTE_E5,  2 },
    };
    MusicTrack t = { .steps = steps, .length = 3, .loop_start = 3 };
    music_play(&t);

    music_tick(); music_tick();  // step 0 done
    music_tick();  // step 1 (REST)
    ASSERT(music_current_step() == 1);
    music_tick(); music_tick();  // step 1 done
    music_tick();  // step 2
    ASSERT(music_current_step() == 2);
}

int main(void) {
    test_init_no_active_track();
    test_play_null_stops();
    test_play_empty_track_stops();
    test_single_step_no_loop_stops();
    test_multi_step_advancement();
    test_loop_back();
    test_stop_while_playing();
    test_rest_note_advances();

    printf("Tests run: %d, Failed: %d\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
```

- [ ] **Step 3: Add the test binary to test/Makefile**

Open `test/Makefile` and inspect its current structure (probably defines test_puzzle_logic and test_layout binaries). Add a parallel target for music. Following the existing pattern (the exact form depends on test/Makefile's structure — read it first), add:

```makefile
test_music: test_music.c ../src/engine/music.c
	gcc -Wall -I../src -o test_music test_music.c ../src/engine/music.c
	./test_music

# Add 'test_music' to the .PHONY list and the all target
```

If test/Makefile has an `all:` target listing the existing binaries, add `test_music` to that list. If it iterates with a wildcard, the new file may be picked up automatically. Read the existing Makefile to match its style.

- [ ] **Step 4: Run the test to verify it fails initially**

Per TDD: we want to see the test fail BEFORE writing implementation. But Task 5.2 already wrote music.c. So actually we want to verify the test PASSES because the implementation already exists. This is a "test backfill" case rather than strict TDD — for the music engine this is acceptable because the state machine logic was straightforward enough to write in one shot and the test exists to verify + lock in behavior.

If you want strict TDD: temporarily comment out `music_tick`'s body, run tests (expect fails), then uncomment and re-run (expect pass). Skip this if you trust the integration.

```bash
make test 2>&1 | tail -20
```

Expected output ends with `Tests run: 8 (or more), Failed: 0`.

If any test fails: read the FAIL line for the assertion that fired, fix music.c, re-run.

- [ ] **Step 5: Verify the full test suite still passes (no regression)**

```bash
make test 2>&1 | tail -10
```

Expected: puzzle_logic (21 tests), layout (26 tests), build_puzzles (13 tests), music (8 tests). All pass.

- [ ] **Step 6: Commit**

```bash
git add test/test_music.c test/Makefile src/engine/music.c
git commit -m "C5.3: host-side tests for music state machine + __SDCC guards

8 tests cover: empty state, NULL/empty track, single-step end, multi-
step advancement, looping, mid-stream stop, REST note. music.c wraps
GBDK includes + emit_note body in __SDCC so host gcc compile works
(SDCC defines __SDCC; host gcc does not). Hardware register writes
remain untested — that's integration-tested in mGBA in Phase 6."
```

### Task 5.4: Wire music_tick into the VBlank ISR

**Files:**
- Modify: `src/main.c` (the `vblank_isr` function at lines 32-37)

- [ ] **Step 1: Add the include + the call**

Open `src/main.c`. Add `#include "engine/music.h"` to the include block at the top alongside the other engine includes:

```c
#include "engine/music.h"
```

Modify `vblank_isr` to call `music_tick()` between `sound_tick()` and `render_flush()`:

```c
static void vblank_isr(void) {
    global_frame_count++;
    anim_tick();
    sound_tick();
    music_tick();
    render_flush();
}
```

Also add a `music_init()` call in `main()` alongside the other init calls:

```c
void main(void) {
    BGP_REG = 0xE4;
    render_init();
    sound_init();
    music_init();
    // ... rest unchanged
```

- [ ] **Step 2: Build**

```bash
make 2>&1 | tail -3
```

Expected: clean build.

- [ ] **Step 3: Verify no audio changes (no track active yet)**

Run in mGBA. Title screen is silent — no scene has called `music_play()` yet so `music_tick` returns early on every frame. SFX (cursor move, A/B/START) still work normally. Walk through to PLAY → solve a group → CH1 SFX (sfx_correct) still plays as expected, because music never claimed CH1.

- [ ] **Step 4: Commit**

```bash
git add src/main.c
git commit -m "C5.4: wire music_tick into VBlank ISR + music_init at boot

No audible change yet — no scene calls music_play. SFX continue to
function (CH1 SFX from sfx_correct etc. still work because music
hasn't claimed CH1). Phase 6 adds the title theme content + scene
hook."
```

### Phase 5 group review

After Tasks 5.1-5.4:

Review the batch from multiple perspectives. Minimum 3 review rounds:
1. **State machine correctness**: all 8 host tests pass. The state machine handles edge cases (empty track, single step, loop wrap, REST, mid-play stop).
2. **No audio regression**: existing SFX continue to work in mGBA. Title silence is preserved (no track active yet).
3. **Code health**: music.c builds for both targets (SDCC for ROM, gcc for host) via the `__SDCC` guard. No dead code in non-GBDK build path.

If round 3 still finds issues, keep going until clean.

---

## Phase 6 — Title theme + integration

**Execution Status:** ⬜ NOT STARTED

**Goal**: Author an 8-bar upbeat title theme (CH1 only, ~5 second loop). Wire scene_title's init/teardown to play/stop the music. Verify in mGBA that the music plays on title, stops cleanly on transition to PLAY, and doesn't interfere with cursor/select SFX.

**Why:** This is the user-facing payoff. Music elevates the title screen from "obviously homebrew" to "feels like a real game."

### Task 6.1: Author the title theme data

**Files:**
- Create: `src/game/title_theme.h`
- Create: `src/game/title_theme.c`

**Composition target:** 8 bars of 4/4 at ~140 BPM. At 60 fps, one beat = 60/(140/60) ≈ 26 frames. We'll use 24-frame beats for tractable math (gives ~150 BPM — still upbeat). One bar = 96 frames. Eight bars = 768 frames ≈ 12.8 seconds per loop.

Simple 8-bar melody pattern: a 2-bar phrase repeated 4 times with the last 2 bars varied (call-and-response). All notes are quarter-note duration (24 frames) for simplicity; some are rests for breathing room. Loop point at step 0 (full loop).

The actual notes — a cheerful C major arpeggio-based motif:

```
Bar 1: C5 E5 G5 C6      (ascending arpeggio)
Bar 2: G5 E5 C5 REST    (descending + rest)
Bar 3: C5 E5 G5 C6      (same as bar 1)
Bar 4: G5 C6 G5 E5      (variation — high to mid)
Bar 5: D5 F5 A5 D5      (arpeggio up an octave)
Bar 6: A5 F5 D5 REST    (descending + rest)
Bar 7: C5 E5 G5 C6      (return to opening)
Bar 8: C6 G5 E5 C5      (resolution — descending to C5)
```

32 steps total, all 24-frame duration → 768 frames = ~12.8 sec loop.

- [ ] **Step 1: Create title_theme.h**

```c
#ifndef GAME_TITLE_THEME_H
#define GAME_TITLE_THEME_H

#include "../engine/music.h"

// 8-bar upbeat title theme, ~12.8 second loop at 60fps. Each step is
// a quarter-note (24 frames at 60fps ≈ 150 BPM). CH1-only. Authored
// by hand in this file because hUGETracker / other trackers are out
// of scope for this project.
extern const MusicTrack TITLE_THEME;

#endif
```

- [ ] **Step 2: Create title_theme.c**

```c
#include "title_theme.h"

// 32 steps × 24 frames per step = 768 frames = ~12.8 seconds at 60 fps.
// Structure:
//   Bars 1-2:  ascending C major arpeggio with descending answer
//   Bars 3-4:  motif repeat with variation
//   Bars 5-6:  arpeggio up a note (Dm) for tension
//   Bars 7-8:  return to C and resolve down
//
// All notes are quarter-notes (24 frames). REST is included for
// breathing room between phrases.
static const MusicStep TITLE_THEME_STEPS[32] = {
    // Bar 1: C E G C↑
    { MUSIC_NOTE_C5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_C6, 24 },
    // Bar 2: G E C rest
    { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_C5, 24 }, { MUSIC_NOTE_REST, 24 },
    // Bar 3: C E G C↑ (repeat opening)
    { MUSIC_NOTE_C5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_C6, 24 },
    // Bar 4: G C↑ G E (variation)
    { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_C6, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_E5, 24 },
    // Bar 5: D F A D (Dm arpeggio)
    { MUSIC_NOTE_D5, 24 }, { MUSIC_NOTE_F5, 24 }, { MUSIC_NOTE_A5, 24 }, { MUSIC_NOTE_D5, 24 },
    // Bar 6: A F D rest
    { MUSIC_NOTE_A5, 24 }, { MUSIC_NOTE_F5, 24 }, { MUSIC_NOTE_D5, 24 }, { MUSIC_NOTE_REST, 24 },
    // Bar 7: C E G C↑ (return)
    { MUSIC_NOTE_C5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_C6, 24 },
    // Bar 8: C↑ G E C (resolution)
    { MUSIC_NOTE_C6, 24 }, { MUSIC_NOTE_G5, 24 }, { MUSIC_NOTE_E5, 24 }, { MUSIC_NOTE_C5, 24 },
};

const MusicTrack TITLE_THEME = {
    .steps      = TITLE_THEME_STEPS,
    .length     = 32,
    .loop_start = 0,  // full-track loop
};
```

- [ ] **Step 3: Add to Makefile SRC list**

In `Makefile`, add `src/game/title_theme.c` to the SRC list:

```makefile
SRC := src/main.c \
       src/engine/render.c src/engine/input.c src/engine/save.c \
       src/engine/sound.c src/engine/anim.c src/engine/music.c \
       src/game/puzzle_logic.c src/game/layout.c \
       src/game/scene_title.c src/game/scene_play.c src/game/scene_win.c \
       src/game/scene_lose.c src/game/scene_all_done.c \
       src/game/title_theme.c
```

- [ ] **Step 4: Build**

```bash
make 2>&1 | tail -3
```

Expected: clean build.

- [ ] **Step 5: Commit**

```bash
git add src/game/title_theme.h src/game/title_theme.c Makefile
git commit -m "C6.1: author title theme — 8-bar CH1 melody, ~12.8s loop

C major motif with Dm tension in the middle bars. All quarter-notes
(24 frames each), 32 steps total, full-track loop. Not yet wired —
Task 6.2 hooks scene_title init/teardown to play/stop."
```

### Task 6.2: Hook scene_title to play/stop the theme

**Files:**
- Modify: `src/game/scene_title.c` (the `title_init` and `title_teardown` functions)

- [ ] **Step 1: Add include**

In `src/game/scene_title.c`, add the include near the top alongside other engine includes:

```c
#include "../engine/music.h"
#include "title_theme.h"
```

- [ ] **Step 2: Start playback in title_init**

Find `title_init()`. After the existing init code (palette reset, save_load, menu setup, redraw_needed = true), add:

```c
    music_play(&TITLE_THEME);
```

The full updated title_init should end with:

```c
    ts.show_confirm = false;
    ts.redraw_needed = true;
    // Default tile layout (font + UI) is already loaded by render_init()
    // from main.c. Plan B's TITLE scene is text-only; Plan C will load
    // title.png art via dynamic VRAM swapping.
    music_play(&TITLE_THEME);
}
```

- [ ] **Step 3: Stop playback in title_teardown**

Find `title_teardown()`. Currently empty body `{}`. Replace with:

```c
static void title_teardown(void) {
    music_stop();
}
```

- [ ] **Step 4: Build**

```bash
make 2>&1 | tail -3
```

- [ ] **Step 5: Verify in mGBA — happy path**

Open ROM in mGBA. Audio must be enabled (Audio menu → unmute if needed).

Expected on TITLE screen entry:
- Music plays — 12.8-second C major loop on CH1.
- Cursor d-pad presses still produce `sfx_move` clicks (CH2) — no audible glitch in the music when SFX fires.
- Selecting NEW GAME (A or START) plays `sfx_select` (CH2) then transitions to PLAY — music STOPS cleanly at the transition. No hanging note.

PLAY scene is silent (no music) — correct.

- [ ] **Step 6: Verify SFX/music coexistence**

On TITLE, rapidly press D-pad up/down to fire many `sfx_move` events. Music continues uninterrupted on CH1 because moves only touch CH2.

Press A to enter PLAY — music stops on transition. Press B in PLAY's quit-confirm to return to TITLE → music restarts from step 0.

- [ ] **Step 7: Verify on DMG**

Switch mGBA to DMG. Music plays correctly on DMG too (no GBC-specific code in music engine). Same SFX coexistence behavior.

- [ ] **Step 8: Commit**

```bash
git add src/game/scene_title.c
git commit -m "C6.2: title scene plays TITLE_THEME on init, stops on teardown

CH1 (music) and CH2 (cursor/select SFX) don't conflict. Music
restarts from step 0 on every TITLE entry (no resume state — that's
fine for a 12-second loop)."
```

### Phase 6 group review

After Tasks 6.1, 6.2:

Review the batch from multiple perspectives. Minimum 3 review rounds:
1. **Audio quality**: music sounds like an intentional melody (not random tones), tempo is upbeat but not frantic, loops cleanly without an audible "click" or pause at the loop boundary.
2. **No SFX clobber**: cursor / select / deselect SFX play over the music without disrupting it. Music continues on CH1 while CH2 plays SFX.
3. **Clean stop**: transitions from TITLE to PLAY (and PLAY back to TITLE via quit) stop and restart the music cleanly — no hanging notes, no garbage tones.

If round 3 still finds issues (e.g., music feels off-tempo, or loop point is awkward), tune the note durations or note sequence in `title_theme.c` and re-commit as `C6.1b: tune title theme — <description>`.

---

## Phase 7 — Final verification + v1.1 release

**Execution Status:** ⬜ NOT STARTED

**Goal**: Cross-check the full build on DMG and GBC modes. Update README to mention GBC color + chiptune. Tag v1.1. Upload the new ROM to the GitHub release.

### Task 7.1: Full DMG playthrough check

- [ ] **Step 1: mGBA DMG mode, clean save**

Switch mGBA Settings → Model = "Game Boy (DMG)". Delete or reset SRAM (Tools → "Manage cheats" doesn't apply, but Tools → "Save data" → "Reset save data" if available, or just delete the .sav file next to the ROM).

- [ ] **Step 2: Title → PLAY → WIN flow**

Boot. Verify:
- Title screen shows in greyscale identical to v1.0
- Title music plays
- Cursor SFX work
- Select NEW GAME → PLAY scene loads
- Music stops cleanly
- Solve all 4 groups of puzzle 1 (BIRDS, COLORS, ACTION, FOOD) — bars stack in greyscale
- WIN scene renders with bars cascading + typewriter stats reveal

- [ ] **Step 3: Title → PLAY → LOSE flow**

Restart. Same flow but intentionally lose: submit 4 wrong groups (any random 4-word selection that doesn't match). LOSE scene renders with all answers in greyscale tier bars.

- [ ] **Step 4: Note any DMG regressions**

If anything visually differs from v1.0 on DMG, that's a bug — file it as a discovery in the plan, fix before Task 7.4 (tag).

### Task 7.2: Full GBC playthrough check

- [ ] **Step 1: mGBA GBC mode, clean save**

Switch model to "Game Boy Color". Reset save.

- [ ] **Step 2: Title → PLAY → WIN flow**

- Title: "GB" banner appears in red/orange accent. Music plays.
- PLAY: solved bars appear in tier colors (yellow/green/blue/purple) as you solve.
- WIN: cascade shows 4 distinctly colored tier bars in sequence.

- [ ] **Step 3: Title → PLAY → LOSE flow**

LOSE scene shows the 4 tier bars in distinct colors.

- [ ] **Step 4: Verify all 30 puzzles still loadable**

Solve a few additional puzzles (P2, P10, P20) to confirm nothing broke for puzzles past the first.

### Task 7.3: Update README to mention GBC + chiptune

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Update the project description**

Find the opening paragraph:

```markdown
A homebrew Game Boy ROM that recreates the [NYT Connections](https://www.nytimes.com/games/connections) puzzle game on the original DMG. 30 puzzles, 4 difficulty tiers per puzzle, full save support, and ear-tuned sound effects.
```

Replace with:

```markdown
A homebrew Game Boy ROM that recreates the [NYT Connections](https://www.nytimes.com/games/connections) puzzle game. 30 puzzles, 4 difficulty tiers per puzzle, full save support, ear-tuned sound effects, and a short upbeat chiptune on the title screen.

Plays on original Game Boy (DMG) in monochrome. On Game Boy Color (GBC) — same ROM, no need to download anything different — the four tier bars render in their faithful NYT yellow/green/blue/purple, and the title banner gets an accent color.
```

- [ ] **Step 2: Update Project status section at bottom**

Find:

```markdown
**v1.0 shipped 2026-05-24.** The game is fully playable end-to-end on any DMG emulator. Hardware verification on real Game Boy hardware via flash cart is deferred to a future release.
```

Replace with:

```markdown
**v1.1 shipped 2026-05-25.** GBC color support + title-screen chiptune added. Backwards-compatible with DMG (same ROM file). Hardware verification on real Game Boy hardware via flash cart is deferred to a future release.
```

- [ ] **Step 3: Commit**

```bash
git add README.md
git commit -m "C7.3: README — note GBC color + title chiptune, bump version to v1.1"
```

### Task 7.4: Tag v1.1 + update GitHub release

- [ ] **Step 1: Tag**

```bash
git tag -a v1.1 -m "Game Boy Connections v1.1

Adds:
  - GBC color support (yellow/green/blue/purple tier bars; red/orange title banner)
  - Title-screen chiptune (8-bar CH1 loop, ~12.8s)
  - Same ROM works on both DMG and GBC

No gameplay changes from v1.0. Save data carries over."
```

- [ ] **Step 2: Push commits + tag**

```bash
git push origin main
git push origin v1.1
```

- [ ] **Step 3: Create GitHub release for v1.1 with the new ROM as asset**

```bash
gh release create v1.1 build/gameboygame.gb --title "Game Boy Connections v1.1" --notes-from-tag
```

- [ ] **Step 4: Verify release**

```bash
gh release view v1.1
```

Expected: shows the v1.1 tag, the title, the body (from tag annotation), and the `gameboygame.gb` asset attached.

### Task 7.5: Plan close-out

- [ ] **Step 1: Update all Phase banners to ✅ SHIPPED with their commit SHAs**

For each Phase 1-7 banner, change `⬜ NOT STARTED` to `✅ SHIPPED at <SHA> on 2026-05-25`. Update the top-of-plan Execution Status table to match.

- [ ] **Step 2: Add a final celebration line**

At the top of the Execution Status section, add:

```markdown
🎉 **Plan complete — v1.1 tagged and released.**
```

- [ ] **Step 3: Commit + push the plan update**

```bash
git add docs/plans/2026-05-25-gameboygame-gbc-color-and-chiptune-plan.md
git commit -m "Plan: mark all phases ✅ SHIPPED — v1.1 complete"
git push origin main
```

---

## End of Plan

When all 7 phases ship and their banners are updated, v1.1 is released. The artifact:

- The same `.gb` ROM works on both DMG (monochrome, music) and GBC (color tier bars, accent banner, music)
- All 60+ host tests passing (now 68+ with the new music tests)
- A git tag `v1.1` marking the release commit
- A new GitHub release with the ROM attached

**What's NOT in this plan (deferred to a future v1.2 / Plan E):**
- Daily-puzzle mode (mentioned in v1.0 deferred list)
- Larger puzzle bank via banked data (mentioned in v1.0 deferred list)
- Stats history screen
- Hint system
- Hardware verification on real Game Boy
- Multi-channel music (CH1 + CH2 harmony, percussion via CH4)
- LSDJ-composed track (out of scope per user preference — see `memory/user_chiptune_learning_interest.md`)
