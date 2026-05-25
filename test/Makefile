# test/Makefile — host-side unit tests via MinGW64 gcc, NOT GBDK
# puzzle_logic.c is pure C99 with no GBDK includes, so it compiles + runs
# on the dev machine. This is the architectural invariant Task 7.9 verifies.
#
# Toolchain (Windows / MSYS2):
#   gcc = /c/msys64/mingw64/bin/gcc.exe (native Windows, no cygwin runtime)
#   Requires TMP/TEMP set to a writable directory — gcc otherwise falls back
#   to C:\WINDOWS which fails on non-admin shells.

# Explicit path to MinGW64 gcc — the plain `gcc` on PATH may resolve to
# the MSYS2 /usr/bin/gcc, which doesn't ship with the standard C headers
# (stdint.h, stdbool.h) in a location -std=c99 searches. The MinGW64
# gcc at /c/msys64/mingw64/bin/gcc.exe is a self-contained Windows
# toolchain with full headers. Falls back to PATH `gcc` if not present.
CC := $(if $(wildcard /c/msys64/mingw64/bin/gcc.exe),/c/msys64/mingw64/bin/gcc.exe,gcc)
CFLAGS := -Wall -Wextra -Werror -I../src -std=c99

# Ensure gcc can find its own subprograms (as.exe, ld.exe, etc.) via
# PATH — otherwise mingw64 gcc falls back to the MSYS2 /usr/bin/as,
# which is an incompatible binutils and silently fails the assembly
# step (compile exits 1 with no error output). Prepending mingw64/bin
# fixes this even when the dev shell has different PATH ordering.
export PATH := /c/msys64/mingw64/bin:$(PATH)

# Export a writable TMPDIR for gcc's internal temp files (the as/ld passes
# write intermediate files there). Without this, gcc -> "Cannot create
# temporary file in C:\WINDOWS\: Permission denied" on a non-admin shell.
export TMPDIR := /tmp
export TMP := /tmp
export TEMP := /tmp

.PHONY: test clean

test: test_puzzle_logic test_layout test_music
	./test_puzzle_logic
	./test_layout
	./test_music

test_puzzle_logic: test_puzzle_logic.c ../src/game/puzzle_logic.c
	@mkdir -p /tmp
	$(CC) $(CFLAGS) -o $@ $^

test_layout: test_layout.c ../src/game/layout.c
	@mkdir -p /tmp
	$(CC) $(CFLAGS) -o $@ $^

test_music: test_music.c ../src/engine/music.c
	@mkdir -p /tmp
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_puzzle_logic test_puzzle_logic.exe test_layout test_layout.exe test_music test_music.exe
