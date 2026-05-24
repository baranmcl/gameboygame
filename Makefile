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
