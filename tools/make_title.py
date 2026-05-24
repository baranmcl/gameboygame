#!/usr/bin/env python3
"""make_title.py — Generate assets/title.png (simple programmatic title screen).

Outputs a 160x144 indexed PNG with the DMG 4-color palette. Title text
rendered as oversized block letters via the font8x8 glyphs from
make_font.py.

NOTE for Plan B: this asset is generated but NOT loaded at runtime —
Plan B Phase 4 walks back the runtime title.png loading (rationale:
360 tiles + 64 font + 32 UI > 256 background tile cap) and renders
TITLE as text-only via the font subsystem. Plan C polishes the
title screen by adopting a different VRAM strategy (e.g., load title
tiles only during SCENE_TITLE, restore font+UI on transition out).
The PNG generation here exists so Plan C inherits a starting point.
"""

import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from make_font import PALETTE, FONT, pack_2bpp_scanline, png_chunk

WIDTH = 160
HEIGHT = 144


def _draw_glyph(px, ch, x0, y0, scale=1):
    if ord(ch) < 0x20 or ord(ch) > 0x5F:
        return
    glyph = FONT[ord(ch) - 0x20]
    for r in range(8):
        row_byte = glyph[r]
        for c in range(8):
            if row_byte & (1 << c):
                for dr in range(scale):
                    for dc in range(scale):
                        y = y0 + r * scale + dr
                        x = x0 + c * scale + dc
                        if 0 <= y < HEIGHT and 0 <= x < WIDTH:
                            px[y][x] = 3


def render_pixels():
    px = [[0] * WIDTH for _ in range(HEIGHT)]
    # Border: dark frame 2 pixels thick
    for r in range(HEIGHT):
        for c in range(WIDTH):
            if r < 2 or r >= HEIGHT - 2 or c < 2 or c >= WIDTH - 2:
                px[r][c] = 3

    # Title "GBCX" at 2x scale, centered horizontally
    title = "GBCX"
    title_x = (WIDTH - len(title) * 8 * 2) // 2
    title_y = 30
    for i, ch in enumerate(title):
        _draw_glyph(px, ch, title_x + i * 16, title_y, scale=2)

    # Subtitle "CONNECTIONS" at 1x, centered
    subtitle = "CONNECTIONS"
    sub_x = (WIDTH - len(subtitle) * 8) // 2
    sub_y = 80
    for i, ch in enumerate(subtitle):
        _draw_glyph(px, ch, sub_x + i * 8, sub_y, scale=1)

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
    print(f"Wrote {output} ({WIDTH}x{HEIGHT}, 2bpp indexed)")


if __name__ == "__main__":
    main()
