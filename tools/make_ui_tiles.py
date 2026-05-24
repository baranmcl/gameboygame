#!/usr/bin/env python3
"""make_ui_tiles.py — Generate assets/ui_tiles.png.

Output: a 128x16 indexed-color PNG with the DMG 4-color palette,
arranged as 16 columns × 2 rows of 8x8 tiles (32 tiles total). Used by
Phase 3 of the gameplay plan to provide UI chrome tiles.

Tile layout (row-major, left to right, top row first):
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
"""

import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from make_font import PALETTE, pack_2bpp_scanline, png_chunk


def _tile_cursor():
    rows = []
    for r in range(8):
        row = []
        for c in range(8):
            if r == 0 or r == 7 or c == 0 or c == 7:
                row.append(3)
            else:
                row.append(0)
        rows.append(row)
    return rows


def _tile_corner_tl():
    return [[3 if (r == 0 or c == 0) else 0 for c in range(8)] for r in range(8)]


def _tile_corner_tr():
    return [[3 if (r == 0 or c == 7) else 0 for c in range(8)] for r in range(8)]


def _tile_corner_bl():
    return [[3 if (r == 7 or c == 0) else 0 for c in range(8)] for r in range(8)]


def _tile_corner_br():
    return [[3 if (r == 7 or c == 7) else 0 for c in range(8)] for r in range(8)]


def _tile_edge_horiz():
    return [[3 if r == 0 else 0 for c in range(8)] for r in range(8)]


def _tile_edge_vert():
    return [[3 if c == 0 else 0 for c in range(8)] for r in range(8)]


def _tile_solid_light():
    return [[0] * 8 for _ in range(8)]


def _tile_selected_highlight():
    # Diagonal-cross pattern in shade 2 — visibly different from default fill
    return [[2 if ((r + c) % 4 == 0) else 0 for c in range(8)] for r in range(8)]


def _tile_pattern_yellow():
    # Sparse dots: dark pixel every 4 in both axes
    return [[3 if (r % 4 == 1 and c % 4 == 1) else 0 for c in range(8)] for r in range(8)]


def _tile_pattern_green():
    # Horizontal stripes: dark on even rows
    return [[3 if r % 2 == 0 else 0 for c in range(8)] for r in range(8)]


def _tile_pattern_blue():
    # Vertical stripes
    return [[3 if c % 2 == 0 else 0 for c in range(8)] for r in range(8)]


def _tile_pattern_purple():
    # 2x2 checker
    return [[3 if ((r // 2) + (c // 2)) % 2 == 0 else 0 for c in range(8)] for r in range(8)]


def _tile_solid_dark():
    return [[3] * 8 for _ in range(8)]


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
    output = sys.argv[1] if len(sys.argv) > 1 else "assets/ui_tiles.png"
    write_png(output, render_pixels())
    print(f"Wrote {output} ({WIDTH}x{HEIGHT}, 2bpp indexed, {len(TILES)} tiles)")


if __name__ == "__main__":
    main()
