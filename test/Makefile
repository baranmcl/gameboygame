# test/Makefile — host-side unit tests via MinGW64 gcc, NOT GBDK
# puzzle_logic.c is pure C99 with no GBDK includes, so it compiles + runs
# on the dev machine. This is the architectural invariant Task 7.9 verifies.
#
# Toolchain (Windows / MSYS2):
#   gcc = /c/msys64/mingw64/bin/gcc.exe (native Windows, no cygwin runtime)
#   Requires TMP/TEMP set to a writable directory — gcc otherwise falls back
#   to C:\WINDOWS which fails on non-admin shells.

CC := gcc
CFLAGS := -Wall -Wextra -Werror -I../src -std=c99

# Export a writable TMPDIR for gcc's internal temp files (the as/ld passes
# write intermediate files there). Without this, gcc -> "Cannot create
# temporary file in C:\WINDOWS\: Permission denied" on a non-admin shell.
export TMPDIR := /tmp
export TMP := /tmp
export TEMP := /tmp

.PHONY: test clean

test: test_puzzle_logic test_layout
	./test_puzzle_logic
	./test_layout

test_puzzle_logic: test_puzzle_logic.c ../src/game/puzzle_logic.c
	@mkdir -p /tmp
	$(CC) $(CFLAGS) -o $@ $^

test_layout: test_layout.c ../src/game/layout.c
	@mkdir -p /tmp
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_puzzle_logic test_puzzle_logic.exe test_layout test_layout.exe
