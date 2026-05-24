# Game Boy Connections — Makefile (Plan A)

# GBDK paths — `lcc` should be on PATH; adjust GBDK_HOME if needed
LCC := lcc

# Compiler / linker flags
# -Wm-yt0x03 → MBC1+RAM+Battery cartridge type byte
# -Wm-yo4   → 4 ROM banks (64KB total)
# -Wm-ya1   → 1 RAM bank (8KB SRAM)
# -Wm-yn"GBCX" → ROM name in cartridge header
# (No -Wm-yc — that would set CGB-compatible header byte. We target DMG only,
#  so we omit it entirely. The cartridge header's CGB flag stays at 0x00 = DMG.)
LCC_FLAGS := -Wm-yt0x03 -Wm-yo4 -Wm-ya1 -Wm-yn"GBCX"

# Asset conversion (png2asset → C tile data)
# Flags rationale:
#   -spr8x8             : emit 8x8 tiles in row-major order (default -spr8x16
#                         pairs vertically-adjacent cells into 8x16 sprite-tiles
#                         and interleaves them — tile 1 ends up as '0', not '!')
#   -sprite_no_optimize : keep empty + duplicate tiles so tile index matches
#                         (ascii - 0x20). Without this, the all-zero space tile
#                         is deduplicated and every subsequent glyph shifts.
#   -tiles_only         : emit only background tile bytes, no metasprite metadata
#   -keep_palette_order : preserve our DMG-ordered palette (light → dark)
#   -noflip             : disable horizontal/vertical flip optimizations
ASSETS_PNG := assets/font.png
ASSETS_GEN := $(patsubst assets/%.png,src/assets_gen/%.c,$(ASSETS_PNG))

src/assets_gen/%.c: assets/%.png
	@mkdir -p $(dir $@)
	png2asset $< -o $@ -spr8x8 -bpp 2 -keep_palette_order -sprite_no_optimize -tiles_only -noflip

# Source files
SRC := src/main.c src/engine/render.c src/engine/input.c src/engine/save.c src/engine/sound.c src/engine/anim.c
SRC += $(ASSETS_GEN)
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
