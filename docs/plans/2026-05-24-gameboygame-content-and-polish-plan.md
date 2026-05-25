# Game Boy Connections — Content + Polish Plan (Plan C of 3)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Finish v1 of Game Boy Connections by completing the polish items Plan B deferred + growing the puzzle bank from 5 to 30. End state is a `.gb` ROM with a polished title screen, visible category labels on solved bars, per-puzzle stats on WIN, ear-tuned SFX, working STATS_FADE animation, and 30 author-written puzzles. Tagged as v1.0 when shipped.

**Architecture:** Most work is GBDK-touching incremental additions to existing subsystems — no new architectural layers. New assets (`assets/font_inv.png` for the inverted font, polished `assets/title.png`) feed through Plan A's png2asset pipeline. Per-puzzle stats use a new lightweight `scene_handoff.h` module (one static struct passed between PLAY and WIN). SFX calibration is empirical (user listening + adjusting frequency bytes). Content authoring is user-driven with validation via Plan B's existing codegen tests.

**Tech Stack:** Same as Plans A + B — GBDK-2020 + MinGW64 gcc 16.1.0 + Python 3 stdlib + GNU Make. No new tooling.

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

**Inherited from Plans A + B.** Plan C builds on top of Plan A (foundation, shipped 2026-05-24 at `d98081c..2cca577`) and Plan B (gameplay, shipped at `7e0f9b7..1f55927`). Plan B's Discoveries section is required reading — it documents 10 distinct gotchas that Plan C executors MUST work around:

1. Cold builds need order-only `$(ASSETS_GEN)` Make prereq (already in Makefile)
2. png2asset emits `BANKREF` macros (cosmetic)
3. **Font lacks lowercase** — all UI text MUST be uppercase
4. **Sprite tile data requires separate `set_sprite_data` call** (background VRAM bank is distinct from sprite VRAM bank even though they overlap addressably)
5. **VBlank ISR's `render_flush` races with main-loop render** — use `redraw_needed` flag pattern on every scene
6. ALL-DATA-LOST prompt is UX-broken on truly-fresh save (Phase 4 fixed)
7. **SDCC `%c`-then-other-format-spec sprintf bug** — NEVER combine `%c` with other format specs in one sprintf call
8. **Scene transitions leak hardware register state** — every scene_init MUST start with `BGP_REG = 0xE4` defensive reset (and similar resets for any other hardware regs the scene touches)
9. **Cursor nav must use VISUAL slot coords**, not absolute cell_idx, when grid repacks
10. **Main-loop iteration counts diverge from VBlank rate** when render passes go long — use `global_frame_count` deltas (ISR-paced) for any time-based main-loop logic

**Build environment.** Same as Plan A's Discoveries. All `Bash` tool calls must prepend `export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:/c/gbdk/bin:$PATH"` to see the GBDK + MinGW64 gcc + MSYS2 make toolchain.

**Pitfalls docs.** Plan A noted these aren't present yet and Plan B agreed. Now after Plan B's 10 discoveries, the cost-benefit has shifted. Plan C SHOULD include a Phase X to formalize Plan A+B's collected pitfalls into `docs/pitfalls/implementation-pitfalls.md` and `docs/pitfalls/testing-pitfalls.md` so future plans inherit them automatically. **Deferred to a later plan** — Plan C focuses on shipping v1; pitfalls doc formalization is its own scoped work.

**Commit cadence.** Continue the Plan A/B pattern: one feature commit per phase + one banner-update commit per phase. Subject format `C<phase>: <imperative summary>`.

**Emulator.** mGBA on Windows. No hardware verification phase (user chose mGBA-only scope at plan time).

**Working directory.** Project root is `c:\Users\baranmcl\Code\gameboygame\`. Relative paths in this plan are relative to root.

---

## Execution Status

**Overall:** 5/6 phases shipped, 0 deferred.

| Phase | Status | Ship SHA(s) | Notes |
|---|---|---|---|
| 1 — Inverted font tiles | ✅ Shipped | `e23b462` | 2026-05-24; category names now readable on all 3 scenes' bars |
| 2 — Title screen art | ✅ Shipped | `6bba585` | 2026-05-24; "GB" 4× banner via custom row-major tile generator; cartridge title also "GB" |
| 3 — Per-puzzle stats on WIN | ✅ Shipped | `af6fb73` | 2026-05-24; TRIES/TIME/ATTEMPT via scene_handoff.h global |
| 4 — SFX pitch calibration | ✅ Shipped | `041d14e` | 2026-05-24; all 9 SFX calibrated + volume hierarchy applied per user ear-test |
| 5 — STATS reveal + flicker eradication | ✅ Shipped | `9cd865b` | 2026-05-24; pivoted from palette swap to incremental typewriter; also fixed scene_play A/B/wrong-submit flickers + dropped redundant SELECT_FLASH |
| 6 — Content authoring + v1 tag | 🚧 In progress | — | user authors puzzles 6-30; final ROM size + v1.0 git tag |

### Deviations

- **Phase 2 (2026-05-24): "GBCX" → "GB" branding rename mid-phase.** User decided the abbreviated "GB" was cleaner than "GBCX". Banner letters scaled from 2× (16×16 px each = 16 tiles total) to 4× (32×32 px each = 32 tiles total) to fill comparable screen space with fewer letters. Cartridge header title also renamed via `-Wm-yn"GB"`. SRAM magic sentinel kept as "GBCX" — purely an SRAM-validity check, not user-visible.
- **Phase 5 (2026-05-24): palette-swap STATS_FADE → typewriter reveal.** BGP is global; the planned palette swap dimmed already-visible chrome (SOLVED! header, bar labels) along with the stats text. Pivoted to per-line typewriter reveal driven from scene_win, with the anim engine still timing the duration. See Phase 5 banner for full rationale.
- **Phase 5 (2026-05-24): scope expanded to include scene_play flicker eradication.** User-surfaced flickers on A-press and B-clear shared the same root cause as the WIN-scene flicker. Rather than ship Phase 5 with the known bugs still in scene_play, fixed them in the same phase: incremental rendering pattern applied to A-toggle, B-clear, wrong-submit header update; redundant `ANIM_SELECT_FLASH` call site removed. See Phase 5 banner for full rationale.

### Discoveries

- **Phase 5 (2026-05-24): the "redraw_needed → render_clear → full re-render" pattern races VBlank and is the universal flicker anti-pattern in this codebase.** `render_clear` zeros all 360 tilemap bytes and sets the dirty flag; the VBlank ISR (`render_flush`) blindly pushes the buffer to VRAM whenever it next fires, which can be mid-update. Hot-path interactions (input → state change → redraw_needed) must write deltas directly via `render_set_tile` / `render_text` to keep the buffer in a consistent valid state at every moment. `render_clear` belongs only in scene `init` and on rare structural transitions (layout shrinks, dialog open/close). Fixed in scene_win (typewriter) and scene_play (A/B/wrong-submit) in Phase 5; pattern documented as the convention for all future scene work.
- **Phase 5 (2026-05-24): "audit the band-aids" when you fix an underlying problem.** `ANIM_SELECT_FLASH` was added in Plan B as visual punctuation for A-press feedback — but the real reason it existed was the redraw-race rendered the cell-color change invisible for 1-2 frames, so users needed a global palette flash to confirm their input registered. Once the race was eliminated and the cell color change became instantaneous, the palette flash was visible-noise the user immediately flagged as flicker. The fix was to remove the call site, not to tune the flash duration. (Other animations — CORRECT_FLASH, CELL_FLASH, WRONG_SHAKE — still earn their keep because they convey state the tile change doesn't directly indicate.)
- **Phase 2 (2026-05-24): png2asset tile-emission order is NOT row-major for 64×32 images.** Plan A's font tile sheet is 128×32 and png2asset emits its tiles in row-major order with the `-spr8x8 -sprite_no_optimize -tiles_only` flags — verified by working ASCII→tile-index mapping. The same flags applied to a 64×32 title image produced visibly broken tile output: each individual 8×8 tile's byte content was correct (representing the right 8×8 region of the source image), but the index→position mapping was scrambled. Could not quickly reverse-engineer png2asset's sprite-iteration logic for this image shape. **Workaround:** `tools/make_title.py` now writes `src/assets_gen/title.{c,h}` directly via a custom ~40-line Python writer that emits row-major explicitly (tile N at image position `(col=N%tiles_wide, row=N//tiles_wide)`, each tile = 8 rows × 2 bytes GB planar 2bpp format, MSB = leftmost pixel). The PNG output is kept for visual debugging and Plan D content swaps; the .c/.h are the authoritative source. Gitignore updated with `src/assets_gen/*` glob + `!src/assets_gen/title.{c,h}` exceptions to track them in git.

---

## Phase 1 — Inverted font tiles

**Execution Status:** ✅ SHIPPED at `e23b462` on 2026-05-24. All 7 tasks complete: `tools/make_font.py` extended with `--inverted` flag; `assets/font_inv.png` generated and added to ASSETS_PNG; `FONT_INV_TILE_BASE = 96` constant + `render_text_inv()` API in render.h; render.c loads font_inv tiles + implements render_text_inv; PLAY/WIN/LOSE `render_bar` helpers updated to take Puzzle* and overlay category names. mGBA visual confirmation received from user — category names readable on all 3 scenes' bars.

**Goal**: Generate a second font tile sheet with inverted palette (light glyphs on dark background) and add a `render_text_inv()` API. Use it to overlay category names on solved bars in PLAY/WIN/LOSE scenes. End state: solved bars show their category name in readable light-on-dark text.

**Why:** Plan B's solved bars use `UI_TILE_SOLID_DARK` (palette index 3 = darkest) as the label background. The font glyphs render in palette index 3 (dark) on index 0 (light) — overlaying them on dark labels makes glyphs invisible. The clean fix is a second font tile sheet with the palette inverted: glyphs become palette index 0 (light) on index 3 (dark). At ~16 bytes per glyph × 64 glyphs = 1024 bytes of additional ROM (negligible).

### Task 1.1: Extend make_font.py to also emit font_inv.png

**Files:**
- Modify: `tools/make_font.py`

- [ ] **Step 1: Add inverted-pixel rendering option**

Refactor `make_font.py` so `render_pixels()` accepts an `inverted` argument. When `inverted=True`, swap the palette index used for foreground vs background pixels.

```python
def render_pixels(inverted=False):
    """Return HEIGHT x WIDTH array of palette indices.

    If inverted, swap fg/bg: glyph pixels become palette index 0 (lightest),
    background pixels become palette index 3 (darkest). Used by Plan C to
    overlay text on dark solved-bar labels.
    """
    bg = 3 if inverted else 0
    fg = 0 if inverted else 3
    px = [[bg] * WIDTH for _ in range(HEIGHT)]
    for glyph_idx, glyph in enumerate(FONT):
        tx = glyph_idx % 16
        ty = glyph_idx // 16
        for row in range(8):
            row_byte = glyph[row]
            for col in range(8):
                if row_byte & (1 << col):
                    px[ty * 8 + row][tx * 8 + col] = fg
    return px
```

- [ ] **Step 2: Update main() to support `--inverted` flag and a second default output path**

Replace `main()`:

```python
def main():
    inverted = "--inverted" in sys.argv
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    default_out = "assets/font_inv.png" if inverted else "assets/font.png"
    output = args[0] if args else default_out
    write_png(output, render_pixels(inverted=inverted))
    label = "inverted" if inverted else "normal"
    print(f"Wrote {output} ({label}, {WIDTH}x{HEIGHT}, 2bpp indexed, {len(FONT)} glyphs)")
```

- [ ] **Step 3: Generate the inverted font PNG**

```bash
python tools/make_font.py --inverted
file assets/font_inv.png
```

Expected: `assets/font_inv.png: PNG image data, 128 x 32, 2-bit colormap, non-interlaced`. Same dimensions and palette as `assets/font.png`; only pixel data differs.

- [ ] **Step 4: Verify the inverted font visually**

```bash
# Inspect both PNGs side by side (open in any image viewer).
# font.png: glyphs are dark on light. font_inv.png: glyphs are light on dark.
```

### Task 1.2: Wire font_inv.png into Makefile + asset pipeline

**Files:**
- Modify: `Makefile`

- [ ] **Step 1: Add font_inv.png to ASSETS_PNG**

Edit the Makefile:

```makefile
# Plan B note kept; ASSETS_PNG list now extended with font_inv.png.
# title.png is generated by tools/make_title.py but NOT
# included here. It would consume ~31KB of ROM space for tile data that
# the runtime never references (Plan B's TITLE scene is text-only;
# Plan C will load title tiles dynamically). To regenerate the .png
# from source, run `python tools/make_title.py` directly.
ASSETS_PNG := assets/font.png assets/ui_tiles.png assets/font_inv.png
```

(Note: this only adds `assets/font_inv.png`. The `assets/title.png` exclusion stays per Plan B Phase 3's Deviation.)

- [ ] **Step 2: Build and verify the asset is generated**

```bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:/c/gbdk/bin:$PATH"
make
ls src/assets_gen/
```

Expected: `font.c font.h font_inv.c font_inv.h ui_tiles.c ui_tiles.h` all present. `font_inv.c` should have a `const uint8_t font_inv_tiles[1024]` array (64 tiles × 16 bytes).

### Task 1.3: Add FONT_INV_TILE_BASE constant + render_text_inv() API

**Files:**
- Modify: `src/engine/render.h`

- [ ] **Step 1: Add the new constant and API declaration**

Edit `src/engine/render.h`. Update the VRAM layout comment block + add the new constant:

```c
// VRAM tile layout:
//   tiles 0..63   = font (ASCII 0x20-0x5F, normal palette)
//   tiles 64..95  = UI chrome (cursor, borders, tier patterns, etc.)
//   tiles 96..159 = font_inv (same glyphs as font, palette inverted —
//                   for text overlaying dark backgrounds like solved bars)
//   tiles 160+    = available for future content (Plan C title art, etc.)
#define UI_TILE_BASE          64
#define FONT_INV_TILE_BASE    96
```

Add the new API after `render_text`:

```c
// Write a null-terminated ASCII string starting at tile column x, row y,
// using the INVERTED font (light glyphs on dark background). Same character
// mapping as render_text. Used for text overlay on dark backgrounds where
// the normal font's dark glyphs would be invisible.
void render_text_inv(uint8_t x, uint8_t y, const char *s);
```

### Task 1.4: Implement render_text_inv() + load font_inv into VRAM

**Files:**
- Modify: `src/engine/render.c`

- [ ] **Step 1: Add the font_inv asset include**

At the top of `render.c`, add the include alongside the existing asset includes:

```c
#include "../assets_gen/font_inv.h"
```

- [ ] **Step 2: Load font_inv tiles into VRAM in render_init()**

Update `render_init()` to load font_inv tiles at the new offset:

```c
void render_init(void) {
    set_bkg_data(0, font_TILE_COUNT, font_tiles);
    set_bkg_data(UI_TILE_BASE, ui_tiles_TILE_COUNT, ui_tiles_tiles);
    set_bkg_data(FONT_INV_TILE_BASE, font_inv_TILE_COUNT, font_inv_tiles);
    render_clear();
}
```

- [ ] **Step 3: Implement render_text_inv()**

Add after `render_text()`:

```c
void render_text_inv(uint8_t x, uint8_t y, const char *s) {
    if (y >= SCREEN_TILES_H) return;
    uint8_t *dst = &tilemap_buf[y * SCREEN_TILES_W + x];
    while (*s && x < SCREEN_TILES_W) {
        char c = *s;
        if (c < 0x20 || c > 0x5F) c = 0x20;
        // Same ASCII-to-tile-index mapping as render_text, but offset by
        // FONT_INV_TILE_BASE so we draw from the inverted-palette tile set.
        *dst++ = (uint8_t)(FONT_INV_TILE_BASE + (c - 0x20));
        s++;
        x++;
    }
    dirty = 1;
}
```

### Task 1.5: Use render_text_inv() for category names on solved bars

**Files:**
- Modify: `src/game/scene_play.c`
- Modify: `src/game/scene_win.c`
- Modify: `src/game/scene_lose.c`

The bar layout in each scene has a 14-tile-wide dark label between two 3-tile patterns. Category name has max 12 chars per `MAX_CATEGORY_NAME_LEN`. Center the name within the 14-tile label.

- [ ] **Step 1: Update scene_play.c's render_solved_bar to overlay category name**

Find the existing function:

```c
static void render_solved_bar(uint8_t tier, uint8_t y) {
    // ...existing pattern + label tile writes...
}
```

Update it to also overlay the category name. We need the puzzle context, so refactor to take it as a parameter:

```c
static void render_solved_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y) {
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

Update all call sites in scene_play.c:

```c
// Before: render_solved_bar(pg_layout.bar_tier[i], (uint8_t)(pg_layout.bar_y[i] + 1));
// After:
render_solved_bar(puzzle, pg_layout.bar_tier[i], (uint8_t)(pg_layout.bar_y[i] + 1));
```

(There should be exactly one call site, in `play_render()`.)

- [ ] **Step 2: Apply the same change to scene_win.c**

`scene_win.c` has its own `render_bar(tier, y)` helper. Update its signature similarly. Need access to the puzzle for category names — but scene_win loads `win_save` which has `current_puzzle_index` already incremented (points to next puzzle), so we want `PUZZLES[win_save.current_puzzle_index - 1]` for the just-completed puzzle's categories.

```c
static void render_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y) {
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

Update the call site in `win_render()`:

```c
// Find this loop:
for (uint8_t i = 0; i < 4 && i < cascade_step; i++) {
    render_bar(i, (uint8_t)(3 + i));
}
// Replace with:
const Puzzle *puzzle = &PUZZLES[win_save.current_puzzle_index - 1];
for (uint8_t i = 0; i < 4 && i < cascade_step; i++) {
    render_bar(puzzle, i, (uint8_t)(3 + i));
}
```

**Edge case:** if `current_puzzle_index == 0` we'd underflow. This shouldn't happen since WIN is only entered after a successful submission incremented the index. But defensive: cap to a valid value:

```c
uint8_t prev_idx = (win_save.current_puzzle_index == 0) ? 0 : (uint8_t)(win_save.current_puzzle_index - 1);
const Puzzle *puzzle = &PUZZLES[prev_idx];
```

- [ ] **Step 3: Apply the same change to scene_lose.c**

Same pattern. `scene_lose.c` has `render_bar(tier, y)`. LOSE doesn't increment `current_puzzle_index`, so just use `PUZZLES[lose_save.current_puzzle_index]`:

```c
static void render_bar(const Puzzle *puzzle, uint8_t tier, uint8_t y) {
    // ... same body as above ...
}

// And in lose_render(), in the reveal loop:
const Puzzle *puzzle = &PUZZLES[lose_save.current_puzzle_index];
for (uint8_t i = 0; i < 4 && i < reveal_step; i++) {
    render_bar(puzzle, i, (uint8_t)(3 + i));
}
```

### Task 1.6: Build + verify in mGBA

**Files:** none

- [ ] **Step 1: Build**

```bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:/c/gbdk/bin:$PATH"
make 2>&1 | tail -10
make size
```

Expected: clean build, ROM at 64KB MBC1+RAM+BATT.

- [ ] **Step 2: Run in mGBA + verify**

Open `build/gameboygame.gb` in mGBA.

Test cases:
- NEW GAME → PLAY puzzle 1 → solve one group (e.g., BIRDS). Expected: solved bar at top now shows "BIRDS" in light text on dark background. Category name should be readable.
- Solve all 4 groups → WIN scene. Each bar in the cascade should show its category name.
- Force a LOSE (fail 4× wrong). LOSE_REVEAL bars should show category names.

### Task 1.7: Commit Phase 1

- [ ] **Step 1: Stage and commit**

```bash
git add tools/make_font.py Makefile assets/font_inv.png src/engine/render.h src/engine/render.c src/game/scene_play.c src/game/scene_win.c src/game/scene_lose.c
git commit -m "C1: inverted font tiles + render_text_inv — category names visible on solved bars"
```

- [ ] **Step 2: Update Phase 1's Execution Status banner above + top-of-plan table**

---

## Phase 2 — Title screen art

**Execution Status:** ✅ SHIPPED at `6bba585` on 2026-05-24. Title scene shows a 4×-scale "GB" banner (64×32 px, 32 tiles loaded at TITLE_TILE_BASE=160) instead of font-rendered text. Cartridge header title also renamed `GBCX → GB` via `-Wm-yn"GB"` Makefile flag (SRAM magic sentinel kept as "GBCX" with explanatory comment — it's just bytes, not branding). **Deviation from plan:** mid-phase user retitled "GBCX → GB", bumping banner letters from 2× to 4× scale (16 → 32 tiles). **One bug surfaced + fixed:** png2asset's tile-emission order for 64×32 images doesn't match its row-major behavior on 128×32 font images. Worked around with a custom ~40-line Python tile-data writer in `tools/make_title.py` that emits row-major explicitly; `src/assets_gen/title.{c,h}` are now checked into git as generated artifacts (gitignore exception added).

**Goal**: Replace the text-only TITLE scene with a graphical logo at the top. End state: TITLE shows a "GBCX CONNECTIONS" logo as graphical art (multiple tiles, larger and more polished than what `render_text` can produce) above the menu, instead of the current text headers.

**Why:** Plan B's title.png was a 360-tile full-screen image that would have consumed ~31KB of ROM. Plan C scopes the title art smaller: a banner-style logo occupying the top ~6 rows of the screen (~120 tiles), leaving the bottom for the menu rendered via the font. Total VRAM usage: 64 (font) + 32 (UI) + 64 (font_inv) + ~120 (title) = ~280 tiles — exceeds the DMG's 256-tile background bank! So we still need a strategy. Two options:

1. **Strategy A (chosen):** Make the title logo smaller — target ≤60 tiles total. The banner can be the text "GBCX" rendered as 2x-scaled solid block-letters in tiles arranged in a 4×2 grid (= 8 tiles each letter × 4 letters = 32 tiles). Plus 4 decorative corner tiles. Total: ~40 tiles, fits easily. Subtitle "CONNECTIONS" stays font-rendered.

2. **Strategy B (deferred):** Dynamic VRAM swap — TITLE scene loads title tiles overwriting UI tiles; restore on transition out. More flexible, but requires the swap-restore logic Plan B Phase 3 explicitly avoided. Defer to a future Plan D.

This phase implements Strategy A.

### Task 2.1: Update tools/make_title.py to emit a small banner

**Files:**
- Modify: `tools/make_title.py`

- [ ] **Step 1: Rewrite to produce a 64×16 PNG (8×2 tile banner)**

The new title is the word "GBCX" drawn at 4×4-tile-per-letter scale = 16 tiles per letter × 4 = 64 tiles. Too big for our budget. Smaller: 2×2-tile-per-letter = 4 tiles per letter × 4 letters = 16 tiles. Plus a 4-tile decorative band = 20 tiles. Tight but works.

Actually for Plan C the simplest path: just hand-design a small 64×16 (8×2 = 16 tile) banner that spells "GBCX" cleanly at this size. Use the same font glyphs but drawn double-height (each glyph spans 2 vertical tiles).

```python
#!/usr/bin/env python3
"""make_title.py — Generate assets/title.png as a small GBCX banner.

Plan C scope: ~16 tiles (64×16 px) for the title banner. Placed at
the top of the TITLE scene via render_set_tile in scene_title.c.
"""

import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from make_font import PALETTE, FONT, pack_2bpp_scanline, png_chunk

WIDTH = 64
HEIGHT = 16

def _draw_glyph_2x(px, ch, x0, y0):
    """Draw a font glyph at 2x scale (16x16 pixels)."""
    if ord(ch) < 0x20 or ord(ch) > 0x5F:
        return
    glyph = FONT[ord(ch) - 0x20]
    for r in range(8):
        row_byte = glyph[r]
        for c in range(8):
            if row_byte & (1 << c):
                for dr in range(2):
                    for dc in range(2):
                        y = y0 + r * 2 + dr
                        x = x0 + c * 2 + dc
                        if 0 <= y < HEIGHT and 0 <= x < WIDTH:
                            px[y][x] = 3

def render_pixels():
    px = [[0] * WIDTH for _ in range(HEIGHT)]
    # "GBCX" at 2x scale: 4 letters × 16 pixels = 64 pixels total. Fits exactly.
    title = "GBCX"
    for i, ch in enumerate(title):
        _draw_glyph_2x(px, ch, i * 16, 0)
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
    body = (
        sig
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"PLTE", plte)
        + png_chunk(b"IDAT", idat)
        + png_chunk(b"IEND", b"")
    )
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_bytes(body)

def main():
    output = sys.argv[1] if len(sys.argv) > 1 else "assets/title.png"
    write_png(output, render_pixels())
    print(f"Wrote {output} ({WIDTH}x{HEIGHT}, ~16 tiles)")

if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Regenerate the title PNG**

```bash
python tools/make_title.py
file assets/title.png
```

Expected: `assets/title.png: PNG image data, 64 x 16, 2-bit colormap, non-interlaced`.

### Task 2.2: Wire title.png back into the build

**Files:**
- Modify: `Makefile`

- [ ] **Step 1: Update ASSETS_PNG**

Remove the "title.png is excluded" comment and add it to the build list:

```makefile
# Plan C: title.png is now small enough (~16 tiles) to include in the
# ROM build. The Plan B exclusion was for the original 360-tile full-
# screen image which would have been 31KB of dead weight.
ASSETS_PNG := assets/font.png assets/ui_tiles.png assets/font_inv.png assets/title.png
```

- [ ] **Step 2: Build and verify title.c is generated + within size budget**

```bash
make
ls -la src/assets_gen/title.*
```

Expected: `title.c` and `title.h` exist. `title_TILE_COUNT` should be ~16 (check title.h).

### Task 2.3: Add TITLE_TILE_BASE + load title tiles into VRAM

**Files:**
- Modify: `src/engine/render.h`
- Modify: `src/engine/render.c`

- [ ] **Step 1: Add the new constant to render.h**

After `FONT_INV_TILE_BASE`:

```c
// Plan C addition: small title banner (~16 tiles) for scene_title.
// Loaded by render_init at offset 160 alongside the other static tile sets.
#define TITLE_TILE_BASE       160
```

- [ ] **Step 2: Load title tiles in render_init**

Update `render_init()` in render.c:

```c
#include "../assets_gen/title.h"

void render_init(void) {
    set_bkg_data(0, font_TILE_COUNT, font_tiles);
    set_bkg_data(UI_TILE_BASE, ui_tiles_TILE_COUNT, ui_tiles_tiles);
    set_bkg_data(FONT_INV_TILE_BASE, font_inv_TILE_COUNT, font_inv_tiles);
    set_bkg_data(TITLE_TILE_BASE, title_TILE_COUNT, title_tiles);
    render_clear();
}
```

### Task 2.4: Render the title banner in scene_title.c

**Files:**
- Modify: `src/game/scene_title.c`

- [ ] **Step 1: Replace text-based "GBCX" header with the graphical banner**

In `title_render`, replace the existing header text with tile-based banner rendering:

```c
// Title banner: 8 tiles wide × 2 tiles tall, centered horizontally.
// Tiles 0..15 in title.png map to TITLE_TILE_BASE + 0..15 in VRAM.
// Layout: row 0 = top half of letters, row 1 = bottom half.
// (8 columns × 2 rows = 16 tiles total.)
{
    const uint8_t banner_w = 8;
    const uint8_t banner_h = 2;
    uint8_t banner_x = (uint8_t)((SCREEN_TILES_W - banner_w) / 2);  // = 6
    uint8_t banner_y = 2;
    for (uint8_t r = 0; r < banner_h; r++) {
        for (uint8_t c = 0; c < banner_w; c++) {
            uint8_t tile_idx = (uint8_t)(r * banner_w + c);
            render_set_tile((uint8_t)(banner_x + c),
                            (uint8_t)(banner_y + r),
                            (uint8_t)(TITLE_TILE_BASE + tile_idx));
        }
    }
}

// Subtitle still uses font rendering
render_text(4, 5, "CONNECTIONS");
```

Remove the old `render_text(7, 2, "GBCX");` line (replaced by the banner block above).

### Task 2.5: Build + verify TITLE scene

**Files:** none

- [ ] **Step 1: Build**

```bash
make
```

- [ ] **Step 2: Verify in mGBA**

Open ROM. Expected on TITLE:
- Top-center: large "GBCX" banner (2x scale, drawn from title tiles)
- Below: "CONNECTIONS" subtitle (1x font, same as Plan B)
- Below: menu items (1-3 items depending on save state)
- Bottom: SOLVED/BEST stats

The title should look noticeably more polished than the Plan B text-only version.

### Task 2.6: Commit Phase 2

- [ ] **Step 1: Stage and commit**

```bash
git add tools/make_title.py Makefile assets/title.png src/engine/render.h src/engine/render.c src/game/scene_title.c
git commit -m "C2: title screen banner — 2x-scale GBCX logo via dedicated title tiles"
```

- [ ] **Step 2: Update Phase 2's Execution Status banner**

---

## Phase 3 — Per-puzzle stats on WIN

**Execution Status:** ✅ SHIPPED at `af6fb73` on 2026-05-24. scene_handoff.h with PuzzleResult struct (tries_used + elapsed_seconds + attempt_number); `last_puzzle_result` defined in main.c. scene_play.c captures stats at the 4/4-solved branch BEFORE pg_save.current_puzzle_fails resets to 0. scene_win.c displays per-puzzle TRIES/TIME/ATTEMPT alongside lifetime STREAK+BEST. mGBA visual confirmation received from user.

**Goal**: WIN scene currently shows lifetime stats (current_streak, best_streak). Add per-puzzle stats (tries used on THIS puzzle + elapsed time). End state: WIN shows TRIES: N/4 + TIME: M:SS for the puzzle just completed, alongside the lifetime streak.

**Why:** Spec §8 WIN mockup shows `TRIES: 3/4`, `TIME: 1:34`, `ATTEMPT: 1` for per-puzzle data. Plan B punted this with a "Plan C polish" comment. The data exists in PLAY's PlayState (tries_remaining, elapsed_seconds) — we just need to hand it off to WIN.

### Task 3.1: Create scene_handoff.h with PuzzleResult struct

**Files:**
- Create: `src/game/scene_handoff.h`

- [ ] **Step 1: Write the header**

```c
// src/game/scene_handoff.h
// Lightweight handoff data passed between scenes via a single static
// instance. Used because adding fields to GameSave for transient post-
// puzzle data would muddy the SRAM schema, and using globals avoids
// threading a result pointer through scene transitions.
//
// Convention: PLAY's submission code populates `last_puzzle_result`
// just before *next_scene = SCENE_WIN. WIN's init reads it.

#ifndef GAME_SCENE_HANDOFF_H
#define GAME_SCENE_HANDOFF_H

#include <stdint.h>

typedef struct {
    uint8_t  tries_used;          // 0..4 (4 - tries_remaining at submit time)
    uint16_t elapsed_seconds;     // copy of PlayState.elapsed_seconds at submit
    uint8_t  attempt_number;      // current_puzzle_fails + 1 (1-based)
} PuzzleResult;

extern PuzzleResult last_puzzle_result;

#endif
```

### Task 3.2: Define the global PuzzleResult instance

**Files:**
- Modify: `src/main.c`

- [ ] **Step 1: Add the definition**

In main.c, alongside the existing global `SCENES[]` and `global_frame_count`, add:

```c
#include "game/scene_handoff.h"

// Defined here so the symbol lives in a fixed TU. Initialized to zeros
// (acceptable — readers always populate before reading on WIN entry).
PuzzleResult last_puzzle_result;
```

### Task 3.3: Populate last_puzzle_result in PLAY's WIN transition

**Files:**
- Modify: `src/game/scene_play.c`

- [ ] **Step 1: Add the include**

```c
#include "scene_handoff.h"
```

- [ ] **Step 2: Populate the result before transition**

In `play_update`, inside the correct-submission branch, just before `*next_scene = SCENE_WIN`:

```c
// Capture per-puzzle stats for WIN scene to display
last_puzzle_result.tries_used = (uint8_t)(4 - ps.tries_remaining);
last_puzzle_result.elapsed_seconds = ps.elapsed_seconds;
last_puzzle_result.attempt_number = (uint8_t)(pg_save.current_puzzle_fails + 1);

*next_scene = SCENE_WIN;
```

(Insert this BEFORE the `*next_scene = SCENE_WIN;` line that already exists in the 4/4-solved branch.)

### Task 3.4: Display per-puzzle stats in WIN scene

**Files:**
- Modify: `src/game/scene_win.c`

- [ ] **Step 1: Add the include**

```c
#include "scene_handoff.h"
```

- [ ] **Step 2: Update win_render to show per-puzzle stats**

In `win_render`, inside the `if (cascade_step >= 4)` block, replace the existing stats display with per-puzzle + lifetime:

```c
if (cascade_step >= 4) {
    char buf[21];

    // Per-puzzle: PUZZLE N / TRIES: U/4 / TIME: M:SS / ATTEMPT: A
    uint8_t puzzle_num = win_save.current_puzzle_index;  // already incremented
    sprintf(buf, "PUZZLE %d", (int)puzzle_num);
    render_text(2, 9, buf);

    sprintf(buf, "TRIES: %d/4", (int)last_puzzle_result.tries_used);
    render_text(2, 10, buf);

    uint16_t mins = (uint16_t)(last_puzzle_result.elapsed_seconds / 60);
    uint16_t secs = (uint16_t)(last_puzzle_result.elapsed_seconds % 60);
    sprintf(buf, "TIME:  %d:%02d", (int)mins, (int)secs);
    render_text(2, 11, buf);

    sprintf(buf, "ATTEMPT: %d", (int)last_puzzle_result.attempt_number);
    render_text(2, 12, buf);

    // Lifetime: STREAK + BEST
    sprintf(buf, "STREAK:%d  BEST:%d",
            (int)win_save.current_streak,
            (int)win_save.best_streak);
    render_text(2, 14, buf);

    render_text(2, 16, "START  NEXT");
    render_text(2, 17, "SELECT TITLE");
}
```

(The "START NEXT" / "SELECT TITLE" prompts may have been on rows 15-16 before; new layout pushes them to 16-17 to fit the additional lines. Adjust if the layout looks crowded.)

### Task 3.5: Add scene_handoff.c to Makefile (if needed)

**Files:**
- Modify: `Makefile`

- [ ] **Step 1: Check whether a .c file is needed**

`scene_handoff.h` has no functions — just the struct typedef and an `extern` declaration. The definition (`PuzzleResult last_puzzle_result;`) is in `main.c`. So no `scene_handoff.c` is needed; nothing to add to SRC.

### Task 3.6: Build + verify

**Files:** none

- [ ] **Step 1: Build**

```bash
make
```

- [ ] **Step 2: Verify in mGBA**

NEW GAME → solve puzzle 1 → WIN scene. Expected:
- 4 bars cascading in (now WITH category names from Phase 1)
- After cascade: PUZZLE N, TRIES: M/4, TIME: 0:XX, ATTEMPT: 1 (first attempt assumed), STREAK + BEST stats
- START → next puzzle
- SELECT → TITLE

To test ATTEMPT > 1: fail a puzzle a few times, then solve it. WIN should show ATTEMPT: 2 or 3 etc.

To test TIME: deliberately wait ~30s on the PLAY scene before solving. WIN should show TIME: 0:30 or similar.

### Task 3.7: Commit Phase 3

- [ ] **Step 1: Stage and commit**

```bash
git add src/game/scene_handoff.h src/main.c src/game/scene_play.c src/game/scene_win.c
git commit -m "C3: per-puzzle stats on WIN — tries used + time + attempt number via scene_handoff"
```

- [ ] **Step 2: Update Phase 3's Execution Status banner**

---

## Phase 4 — SFX pitch calibration

**Execution Status:** ✅ SHIPPED at `041d14e` on 2026-05-24. All 9 SFX calibrated from placeholder bytes to real notes (C-major arpeggio for correct/win, F→D for wrong, F→D→A for lose, G→E for skip). User ear-test in mGBA flagged cursor-move as abrasive; cascaded volume reductions across all SFX with hierarchy cursor(4) < action(6) < game-events(9). Reject noise also softened from 15-bit metallic static to 4-vol slower noise pattern. NOTE_XX_LO/HI #defines added for readability. **Deviation from plan:** the plan recommended ear-iteration after calibration; the user flagged volume issues before pitch issues, leading to a coordinated volume-softening pass that wasn't in the original task list.

**Goal**: Replace the placeholder NR13/NR14 frequency bytes in `sound.c` with values that produce actual musical notes per spec §13. End state: each multi-note SFX (correct/wrong/win/lose/skip) plays recognizable note progressions instead of arbitrary pitches.

**Why:** Plan A Phase 5's frequency values were explicitly placeholder per spec §13. The byte values were guesses that produce "audibly distinct sounds" but not particular notes. Calibration uses the GB's frequency formula: `hz = 131072 / (2048 - x)` where `x` is the 11-bit value split across NR13 (low 8 bits) and NR14 (high 3 bits in bits 0-2).

**Frequency table (used by all calibrated SFX below):**

| Note | Hz | x value | NR13 (low) | NR14 high bits (trigger+freq) |
|---|---|---|---|---|
| C4 | 261.6 | 1547 | 0x0B | bits 0-2 = 0x6 → NR14 = 0x86 |
| D4 | 293.7 | 1602 | 0x42 | NR14 = 0x86 |
| E4 | 329.6 | 1650 | 0x72 | NR14 = 0x86 |
| F4 | 349.2 | 1673 | 0x89 | NR14 = 0x86 |
| G4 | 392.0 | 1714 | 0xB2 | NR14 = 0x86 |
| A4 | 440.0 | 1750 | 0xD6 | NR14 = 0x86 |
| B4 | 493.9 | 1783 | 0xF7 | NR14 = 0x86 |
| C5 | 523.3 | 1798 | 0x06 | NR14 = 0x87 |
| D5 | 587.3 | 1825 | 0x21 | NR14 = 0x87 |
| E5 | 659.3 | 1849 | 0x39 | NR14 = 0x87 |
| F5 | 698.5 | 1860 | 0x44 | NR14 = 0x87 |
| G5 | 784.0 | 1881 | 0x59 | NR14 = 0x87 |
| A5 | 880.0 | 1899 | 0x6B | NR14 = 0x87 |
| B5 | 987.8 | 1915 | 0x7B | NR14 = 0x87 |
| C6 | 1046.5 | 1923 | 0x83 | NR14 = 0x87 |
| E6 | 1318.5 | 1948 | 0x9C | NR14 = 0x87 |

(Values computed from `x = round(2048 - 131072/hz)`; verified by inspection. NR14 bits 0-2 = `(x >> 8) & 0x7`, plus bit 7 = 1 (trigger), plus bit 6 = 0 (no length-counter); for x in 1792-2047 the high bits are 0x7, so NR14 = 0b10000111 = 0x87. For x in 1536-1791 the high bits are 0x6, so NR14 = 0x86.)

### Task 4.1: Calibrate sfx_correct (rising C major arpeggio)

**Files:**
- Modify: `src/engine/sound.c`

Spec target: "rising 3-note arpeggio (major triad)". Use C5 → E5 → G5.

- [ ] **Step 1: Replace placeholder values for sfx_correct**

In `sound.c`, find:

```c
void sfx_correct(void) {
    active = SFX_PLAYING_CORRECT;
    step = 0;
    step_remaining = 5;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0xC1;   // placeholder
    NR14_REG = 0x87;
}
```

Replace the NR13_REG value with the calibrated C5 byte:

```c
void sfx_correct(void) {
    active = SFX_PLAYING_CORRECT;
    step = 0;
    step_remaining = 5;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x06;   // C5 = 523Hz
    NR14_REG = 0x87;
}
```

Also fix the chained notes in `sound_tick`'s `SFX_PLAYING_CORRECT` case:

```c
case SFX_PLAYING_CORRECT: {
    if (step == 1) {
        NR13_REG = 0x39; NR14_REG = 0x87;  // E5
        step_remaining = 5;
    } else if (step == 2) {
        NR13_REG = 0x59; NR14_REG = 0x87;  // G5
        step_remaining = 5;
    } else {
        active = SFX_NONE;
    }
    break;
}
```

### Task 4.2: Calibrate sfx_wrong (F5 → D5 descending)

**Files:**
- Modify: `src/engine/sound.c`

Spec target: descending 2-note "uh-oh". Use F5 → D5.

- [ ] **Step 1: Update sfx_wrong**

```c
void sfx_wrong(void) {
    active = SFX_PLAYING_WRONG;
    step = 0;
    step_remaining = 6;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x44;   // F5
    NR14_REG = 0x87;
}
```

And the SFX_PLAYING_WRONG case in sound_tick:

```c
case SFX_PLAYING_WRONG: {
    if (step == 1) {
        NR13_REG = 0x21; NR14_REG = 0x87;  // D5
        step_remaining = 6;
    } else {
        active = SFX_NONE;
    }
    break;
}
```

### Task 4.3: Calibrate sfx_win (C5 → E5 → G5 → C6 sustained)

**Files:**
- Modify: `src/engine/sound.c`

Spec target: celebratory arpeggio + held resolve. Use C5 → E5 → G5 → C6.

- [ ] **Step 1: Update sfx_win**

```c
void sfx_win(void) {
    active = SFX_PLAYING_WIN;
    step = 0;
    step_remaining = 8;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x06;   // C5
    NR14_REG = 0x87;
}
```

And SFX_PLAYING_WIN in sound_tick:

```c
case SFX_PLAYING_WIN: {
    if (step == 1)      { NR13_REG = 0x39; NR14_REG = 0x87; step_remaining = 8; }   // E5
    else if (step == 2) { NR13_REG = 0x59; NR14_REG = 0x87; step_remaining = 8; }   // G5
    else if (step == 3) { NR13_REG = 0x83; NR14_REG = 0x87; step_remaining = 16; }  // C6 sustain
    else                { active = SFX_NONE; }
    break;
}
```

### Task 4.4: Calibrate sfx_lose (F5 → D5 → A4 descending sad)

**Files:**
- Modify: `src/engine/sound.c`

Spec target: descending sad chord pair. Use F5 → D5 → A4.

- [ ] **Step 1: Update sfx_lose**

```c
void sfx_lose(void) {
    active = SFX_PLAYING_LOSE;
    step = 0;
    step_remaining = 12;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x44;   // F5
    NR14_REG = 0x87;
}
```

And SFX_PLAYING_LOSE in sound_tick:

```c
case SFX_PLAYING_LOSE: {
    if (step == 1)      { NR13_REG = 0x21; NR14_REG = 0x87; step_remaining = 12; }  // D5
    else if (step == 2) { NR13_REG = 0xD6; NR14_REG = 0x86; step_remaining = 16; }  // A4
    else                { active = SFX_NONE; }
    break;
}
```

### Task 4.5: Calibrate sfx_skip (G4 → E4 neutral descent)

**Files:**
- Modify: `src/engine/sound.c`

Spec target: neutral 2-note descent. Use G4 → E4.

- [ ] **Step 1: Update sfx_skip**

```c
void sfx_skip(void) {
    active = SFX_PLAYING_SKIP;
    step = 0;
    step_remaining = 10;
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0xB2;   // G4
    NR14_REG = 0x86;
}
```

And SFX_PLAYING_SKIP in sound_tick:

```c
case SFX_PLAYING_SKIP: {
    if (step == 1) {
        NR13_REG = 0x72; NR14_REG = 0x86;  // E4
        step_remaining = 10;
    } else {
        active = SFX_NONE;
    }
    break;
}
```

### Task 4.6: Calibrate single-tick SFX (move, select, deselect)

**Files:**
- Modify: `src/engine/sound.c`

These don't have multi-step note progressions, just need pleasant pitches:
- `sfx_move`: very brief tick. C6 (highest = most attention-getting per spec)
- `sfx_select`: brief upward tick. E5 (mid-range)
- `sfx_deselect`: brief downward tick. C5 (lower than select)

- [ ] **Step 1: Update the single-tick SFX**

```c
void sfx_move(void) {
    NR21_REG = 0x80;
    NR22_REG = 0x84;
    NR23_REG = 0x83;   // C6
    NR24_REG = 0x87;
    active = SFX_NONE;
}

void sfx_select(void) {
    NR21_REG = 0x80;
    NR22_REG = 0xA4;
    NR23_REG = 0x39;   // E5
    NR24_REG = 0x87;
    active = SFX_NONE;
}

void sfx_deselect(void) {
    NR21_REG = 0x80;
    NR22_REG = 0xA4;
    NR23_REG = 0x06;   // C5
    NR24_REG = 0x87;
    active = SFX_NONE;
}
```

`sfx_reject` uses CH4 (noise channel), no frequency value — leave unchanged.

### Task 4.7: Build + ear-test each SFX

**Files:** none

- [ ] **Step 1: Build**

```bash
make
```

- [ ] **Step 2: Listen in mGBA**

Make sure mGBA audio is enabled (Audio/Video menu).

Trigger each SFX through gameplay:
- Cursor move → sfx_move (high tick)
- A on a cell → sfx_select (mid tick)
- A again same cell → sfx_deselect (low tick)
- START with <4 selected → sfx_reject (noise buzz, unchanged from Plan A)
- Submit correct group → sfx_correct (C-E-G rising)
- Submit wrong group → sfx_wrong (F-D descending)
- Win all 4 → sfx_win (C-E-G-C celebratory)
- Lose all tries → sfx_lose (F-D-A sad)
- Skip puzzle (LOSE menu) → sfx_skip (G-E neutral)

Listen for recognizable musical notes. The arpeggios should sound like rising C-major scales, descending notes for wrong/lose/skip, etc.

**Iteration note:** if a note sounds off, recompute using the formula. The byte values above are best-effort calibration from the freq table; real DMG hardware may sound slightly different (Plan A spec §13 noted this), but on emulator they should be close to the target pitches.

### Task 4.8: Commit Phase 4

- [ ] **Step 1: Stage and commit**

```bash
git add src/engine/sound.c
git commit -m "C4: SFX pitch calibration — real C-major arpeggio for correct, F5→D5 wrong, etc."
```

- [ ] **Step 2: Update Phase 4's Execution Status banner**

---

## Phase 5 — STATS_FADE animation

**Execution Status:** ✅ SHIPPED on 2026-05-24 — scope expanded beyond plan.

**Deviation from plan (visual approach):** Plan specified a 2-stage BGP palette-swap fade for stats. Built it, but BGP is a *global* register — the fade also dimmed the already-visible "SOLVED!" header and the bar category names, which felt broken rather than polished. Pivoted to a **per-line typewriter reveal**: stats lines appear one at a time over 30 frames (5 frames per line, 6 lines total) via the existing `redraw_needed` mechanism keyed off a `stats_step` counter computed from `global_frame_count`. The animation engine still drives the timing (`ANIM_STATS_FADE` keeps its frame counter via `tick_generic_stub`); only the visual implementation moved into the scene. Visual stays bounded to the stats region, no cross-contamination of the already-rendered chrome.

**Deviation from plan (flicker fix in scene_win):** Initial typewriter implementation re-rendered the full WIN scene on every step (`render_clear` + full re-render of header + bars + stats). The 6 rapid `render_clear`s overshot the VBlank window — `render_flush` could sample the partially-cleared tilemap mid-update, producing visible flicker. Refactored `win_render` to be purely additive: `render_clear` + "SOLVED!" header moved to `win_init` (one-shot); render hot path tracks `last_rendered_cascade_step` and `last_rendered_stats_step` and only paints NEW content per step. Buffer stays in a consistent valid-screen state at every moment, so VBlank can sample it whenever it wants.

**Deviation from plan (additional flicker fix in scene_play):** While verifying Phase 5, user surfaced two pre-existing flickers in the PLAY scene that had the same root cause as the WIN-scene fix:

1. **A-press (cell selection) flickered the whole screen** because `redraw_needed = true` → `render_clear` + full re-render of header + bars + 16 cells. Fix: rewrote A-press to call `render_cell` directly for just the toggled cell. Same fix applied to B-clear-selections (only repaint previously-selected cells) and wrong-submit (only repaint header line — `tries` count is the only thing that changed). Full-redraw path preserved for layout changes (correct submit → fewer cells) and dialog open/close where it's load-bearing. Extracted `render_header()` and `render_board()` helpers so the full and incremental paths stay DRY.

2. **`ANIM_SELECT_FLASH` (whole-screen BGP inversion for 4 frames on every A-press)** was a Plan B band-aid for the redraw race — it gave "your A-press registered" feedback at a time when the cell color change was sometimes invisible for 1-2 frames during the redraw. Now that the cell paints directly with no clear, the cell-fill swap from `UI_TILE_FILL_LIGHT` to `UI_TILE_FILL_SEL` is the feedback; the global palette flash was redundant noise piled on top. Removed the call site in scene_play (left the enum + tick handler in place — only ~12 bytes ROM, might be reused for menu-pulse on title).

**Discoveries:**

- **The "redraw_needed → render_clear → full re-render" pattern is the universal flicker anti-pattern in this codebase.** `render_clear` zeros 360 tilemap bytes and sets `dirty=1`. The VBlank ISR (`render_flush`) blindly pushes the dirty buffer to VRAM the next time it fires, which can be mid-update. Any scene that uses this pattern for hot-path interactions (input → redraw_needed → render_clear) will flicker.
- **Rule going forward**: `render_clear` belongs in scene `init` and on rare structural transitions (e.g., layout shrinks, dialog open/close). All other state changes should write deltas directly via `render_set_tile` / `render_text` — the buffer stays in a valid state at every moment, VBlank can sample it any time without artifacts.
- **Pattern**: when you fix the underlying problem, audit the band-aids. `SELECT_FLASH` was a workaround for the very race we just eliminated; once gone, the band-aid became visible noise. CORRECT_FLASH and CELL_FLASH still earn their keep (they convey state — group correct/wrong — that the tile change doesn't directly indicate).
- **SDCC warning "EVELYN the modified DOG"**: harmless optimizer note about loop reordering when conditional comparisons involve unsigned arithmetic. Appears in scenes/anim throughout. Not a real issue.

**Ship SHA:** `9cd865b` on 2026-05-24.

---

**Plan-as-written tasks below.** Plan recorded the palette-swap approach; final code differs per the deviation note above. Tasks kept for archaeology — they describe a real (but rejected) approach to the same problem.


**Goal**: Replace the `ANIM_STATS_FADE` stub in `engine/anim.c` with a real 2-stage palette swap. End state: after WIN's BAR_CASCADE completes, the stats text fades in via two palette stages (invisible → mid-shade → full-shade) instead of appearing instantly.

**Why:** Plan B punted STATS_FADE because the scene worked fine without it. Plan C adds the polish for v1. Spec §9 says "Stats text via 2-stage palette swap."

**Approach:** The fade is achieved by manipulating BGP (background palette) so that the dark shade (index 3, used by font glyphs) appears as light first, then mid, then full dark. The text is rendered to the tilemap unchanged; only BGP changes per frame.

- Stage 0 (frames 0-9): BGP = 0xE0 → palette `11 10 00 00`. Index 3 = shade 3 (visible). Index 2 = shade 2. Indices 0+1 both = shade 0. Text still visible but background-blending. (Actually for a fade-in, we want INDEX 3 to start LIGHT and become DARK.)

Let me redo the palette math: BGP layout is bits 7-6 = shade for color 3, bits 5-4 = shade for color 2, bits 3-2 = shade for color 1, bits 1-0 = shade for color 0. Standard `0xE4 = 11 10 01 00` means index 3 → shade 3 (dark), index 0 → shade 0 (light).

For a fade-in on text rendered at index 3 (dark glyph on light bg), we want index 3 to start as shade 0 (invisible) and end at shade 3 (visible).

- Stage 0: BGP = 0x24 → `00 10 01 00`. Index 3 = shade 0. Text invisible. (Indices 0, 1, 2 unchanged.)
- Stage 1: BGP = 0xA4 → `10 10 01 00`. Index 3 = shade 2. Text mid-visible.
- Stage 2: BGP = 0xE4 → `11 10 01 00`. Index 3 = shade 3. Text full-visible.

### Task 5.1: Implement tick_stats_fade

**Files:**
- Modify: `src/engine/anim.c`

- [ ] **Step 1: Add the tick handler**

In `anim.c`, alongside the other `tick_*` functions:

```c
// STATS_FADE: 2-stage palette swap fade-in on background index 3 (dark
// glyphs become progressively visible). Duration typically 20 frames =
// ~333ms. Stage 0 = frames 0-9 (index 3 invisible), stage 1 = frames
// 10+ (index 3 mid-shade). On end, restores full palette.
static void tick_stats_fade(void) {
    if (active.frame >= active.duration) {
        BGP_REG = 0xE4;  // full normal palette
        active.type = ANIM_NONE;
        return;
    }
    // First half of duration: invisible. Second half: mid-shade.
    uint16_t half = (uint16_t)(active.duration / 2);
    if (active.frame < half) {
        BGP_REG = 0x24;  // index 3 = shade 0 (invisible)
    } else {
        BGP_REG = 0xA4;  // index 3 = shade 2 (mid)
    }
    active.frame++;
}
```

- [ ] **Step 2: Wire into the anim_tick dispatch**

Find the `case ANIM_STATS_FADE:` line and replace `tick_generic_stub` with `tick_stats_fade`:

```c
case ANIM_STATS_FADE:     tick_stats_fade(); break;
```

### Task 5.2: Trigger STATS_FADE from scene_win after BAR_CASCADE

**Files:**
- Modify: `src/game/scene_win.c`

- [ ] **Step 1: Add a state flag for triggering STATS_FADE once**

Add a static:

```c
static bool stats_fade_triggered;
```

Reset to false in `win_init`:

```c
stats_fade_triggered = false;
```

- [ ] **Step 2: Trigger STATS_FADE right when cascade ends**

In `win_update`, modify the cascade-done detection:

```c
if (anim_current() == ANIM_BAR_CASCADE) {
    // ... existing cascade_step logic ...
    return;
}

// Cascade just ended — kick off the stats fade-in (once).
if (cascade_step >= 4 && !stats_fade_triggered) {
    stats_fade_triggered = true;
    anim_start(ANIM_STATS_FADE, 0, 20);
    redraw_needed = true;
}

if (anim_is_playing()) return;
```

(The trigger happens once. While STATS_FADE is playing, `anim_is_playing()` is true so input stays gated.)

### Task 5.3: Build + verify

**Files:** none

- [ ] **Step 1: Build**

```bash
make
```

- [ ] **Step 2: Verify in mGBA**

Solve a puzzle → WIN. Expected:
- 4 bars cascade in (with category names from Phase 1)
- Cascade ends → stats text fades in over ~333ms (2 stages of palette)
- After fade, stats are fully visible at normal contrast
- START/SELECT inputs work after fade

The fade should be subtle — bars and text don't pop in instantly. If the effect is too subtle to notice, that's still acceptable; it's polish, not load-bearing.

### Task 5.4: Commit Phase 5

- [ ] **Step 1: Stage and commit**

```bash
git add src/engine/anim.c src/game/scene_win.c
git commit -m "C5: STATS_FADE animation — 2-stage palette fade-in for WIN scene stats"
```

- [ ] **Step 2: Update Phase 5's Execution Status banner**

---

## Phase 6 — Content authoring + v1 tag

**Execution Status:** ⬜ NOT STARTED

**Goal**: Grow the puzzle bank from 5 to 30 puzzles via user-authored content, then tag the repo as v1.0. End state: `content/puzzles.json` has 30 valid puzzles (validated by Plan A Phase 8's existing codegen tests), the ROM bakes them all in, and `git tag v1.0` marks the v1 release commit.

**Why:** Spec §6 calls for ~30 puzzles for v1. Plan A shipped 5 samples. Plan C closes the gap. User-authored content per scope decision; agent assists with validation.

### Task 6.1: User authors puzzles 6-15 (10 puzzles)

**Files:**
- Modify: `content/puzzles.json`

- [ ] **Step 1: User writes the new puzzle entries**

The user appends 10 new puzzle objects to the `"puzzles"` array. Each entry MUST satisfy Plan A's 6 validation rules (see `tools/build_puzzles.py` for the rules). Skeleton for each:

```json
{
  "id": 6,
  "categories": [
    { "tier": "yellow", "name": "CATEGORY 1", "words": ["WORD1", "WORD2", "WORD3", "WORD4"] },
    { "tier": "green",  "name": "CATEGORY 2", "words": ["WORD5", "WORD6", "WORD7", "WORD8"] },
    { "tier": "blue",   "name": "CATEGORY 3", "words": ["WORD9", "WORD10", "WORD11", "WORD12"] },
    { "tier": "purple", "name": "CATEGORY 4", "words": ["WORD13", "WORD14", "WORD15", "WORD16"] }
  ]
}
```

**Authoring constraints (enforced by codegen validator):**
- `id` MUST be sequential starting at 6 (after the existing 5)
- Each puzzle has exactly 4 categories
- Tiers MUST be exactly `{yellow, green, blue, purple}`, no duplicates
- Each category has exactly 4 words
- Category name: uppercase A-Z + space only, length 1-12
- Words: uppercase A-Z only, length 1-8
- No duplicate words within a single puzzle (case-insensitive)
- No category-name = word collision

Tier convention: yellow = easiest, purple = hardest (the player is told this; design difficulty accordingly).

- [ ] **Step 2: Run validation**

```bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:/c/gbdk/bin:$PATH"
make test
```

Expected: all tests pass (21 puzzle_logic + 26 layout + 13 codegen). If codegen tests fail, the JSON has a rule violation — the error message identifies which puzzle and which rule.

- [ ] **Step 3: Build and verify in mGBA**

```bash
make
```

Open ROM. NEW GAME → solve puzzle 1 → next puzzle → ... → reach puzzle 6+. Each new puzzle should render correctly with its category names and word grid.

- [ ] **Step 4: Commit the batch**

```bash
git add content/puzzles.json
git commit -m "C6.1: author puzzles 6-15 (10 new puzzles)"
```

### Task 6.2: User authors puzzles 16-25 (10 puzzles)

**Files:**
- Modify: `content/puzzles.json`

- [ ] **Step 1: Repeat the Task 6.1 process for 10 more puzzles**

Same authoring constraints. IDs 16-25 sequential.

- [ ] **Step 2: Validate + commit**

```bash
make test
git add content/puzzles.json
git commit -m "C6.2: author puzzles 16-25 (10 new puzzles)"
```

### Task 6.3: User authors puzzles 26-30 (final 5)

**Files:**
- Modify: `content/puzzles.json`

- [ ] **Step 1: Author the final 5 puzzles**

IDs 26-30. Same rules.

- [ ] **Step 2: Validate + commit**

```bash
make test
git add content/puzzles.json
git commit -m "C6.3: author puzzles 26-30 (final 5 puzzles for v1 bank)"
```

### Task 6.4: Final v1 verification

**Files:** none

- [ ] **Step 1: Full ROM size check**

```bash
make clean && make && make size
file build/gameboygame.gb
```

Expected: ROM at 64KB MBC1+RAM+BATT. 30 puzzles × ~200 bytes ≈ 6KB of puzzle data; total ROM well under the 64KB cap.

- [ ] **Step 2: Full test suite**

```bash
make test
```

Expected: all 60+ tests pass (21 puzzle_logic + 26 layout + 13 codegen + any new tests added).

- [ ] **Step 3: ALL_DONE flow check**

Solve puzzles 1-30 in sequence (or use mGBA's memory viewer to set `current_puzzle_index = 29` then solve one). After puzzle 30: ALL_DONE scene appears with `YOU SOLVED ALL 30!` (verify the number reflects `NUM_PUZZLES = 30`).

- [ ] **Step 4: Cycle restart works**

ALL_DONE → START → TITLE. Should show CONTINUE pointing back to puzzle 1.

### Task 6.5: Tag v1.0 in git

**Files:** none

- [ ] **Step 1: Tag the release**

```bash
git tag -a v1.0 -m "Game Boy Connections v1.0 — first complete release with all 30 puzzles"
```

- [ ] **Step 2: Verify the tag**

```bash
git tag -l
git show v1.0 --stat | head -20
```

Expected: `v1.0` appears in tag list; the show output displays the tag message + the tagged commit's stats.

- [ ] **Step 3: Push the tag (optional, depends on user choice for remote)**

```bash
git push origin v1.0
```

(Skip this step if the user wants to keep the tag local-only.)

### Task 6.6: Update Phase 6 banner + close out Plan C

- [ ] **Step 1: Update Phase 6 banner above to ✅ SHIPPED**

Reflect the shipped state in the per-phase banner and the top-of-plan Execution Status table.

- [ ] **Step 2: Add a final celebration line to the Execution Status table**

Below the table, add:

```markdown
🎉 **Plan C complete — v1.0 tagged.**
```

---

## End of Plan C

When all 6 phases ship and their banners are updated, Plan C is complete and v1.0 is tagged. The artifact at this point:

- A `.gb` ROM with 30 puzzles, polished title screen, visible category names on solved bars, per-puzzle stats on WIN, ear-tuned SFX, and STATS_FADE animation
- All 60+ host-side tests passing
- The complete game ready to share with friends (emulator) or extend in a future Plan D (hardware verification, additional polish, daily-puzzle mode, hint system, etc.)
- A git tag `v1.0` marking the v1 release commit

**What's NOT in Plan C (deferred to Plan D / v1.1+):**
- Hardware verification on real DMG via flash cart
- Title-screen chiptune music
- Daily-puzzle mode
- Hint system
- Stats history screen
- Cells-only shake fallback
- Save migration system
- Pitfalls docs formalization (Plan A+B's collected gotchas → `docs/pitfalls/{implementation,testing}-pitfalls.md`)
- Word deduplication across puzzles
- Automated playthrough scripts (BGB Lua)
- Multiple save slots / per-player profiles
