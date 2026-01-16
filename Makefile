.PHONY: all asm tests examples emulator clean clean-all raylib-build raylib-clean

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    PLATFORM := macos
endif
ifeq ($(UNAME_S),Linux)
    PLATFORM := linux
endif
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
endif

RAYLIB_SRC_PATH = thirdparty/raylib/src
RAYLIB_LIB = $(RAYLIB_SRC_PATH)/libraylib.a

ifeq ($(PLATFORM),macos)
    CC = clang
    EXE_EXT =
    RAYLIB_LDFLAGS = $(RAYLIB_LIB) \
        -framework OpenGL \
        -framework Cocoa \
        -framework IOKit \
        -framework CoreAudio \
        -framework CoreVideo
endif

ifeq ($(PLATFORM),linux)
    CC = gcc
    EXE_EXT =
    RAYLIB_LDFLAGS = $(RAYLIB_LIB) \
        -lGL -lm -lpthread -ldl -lrt -lX11
endif

ifeq ($(PLATFORM),windows)
    CC = gcc
    EXE_EXT = .exe
    RAYLIB_LDFLAGS = $(RAYLIB_LIB) \
        -static -lopengl32 -lgdi32 -lwinmm -lshell32
endif

CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Ivm -Ithirdparty
RAYLIB_INCLUDE = -I$(RAYLIB_SRC_PATH)

BINDIR = bin

all: tests asm emulator examples

$(BINDIR):
	mkdir -p $@

tests: $(BINDIR) vm/*.c vm/*.h tests/*.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/tiny16-tests$(EXE_EXT) tests/tiny16.c
	$(BINDIR)/tiny16-tests$(EXE_EXT) | column -t | paste - - -

asm: $(BINDIR) vm/*.c vm/*.h asm/*.h asm/*.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/tiny16-asm$(EXE_EXT) asm/tiny16.c

emulator: $(BINDIR) vm/*.c vm/*.h emulator/*.c $(RAYLIB_LIB) | $(BINDIR)
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -o $(BINDIR)/tiny16-emu$(EXE_EXT) emulator/tiny16.c $(RAYLIB_LDFLAGS)

EXAMPLES := $(wildcard examples/*.asm)
examples: asm
	@for F in $(EXAMPLES); do \
		$(BINDIR)/tiny16-asm$(EXE_EXT) $$F $(BINDIR)/$$(basename $$F .asm).tiny16; \
	done

$(RAYLIB_LIB):
	$(MAKE) -C $(RAYLIB_SRC_PATH) PLATFORM=PLATFORM_DESKTOP

raylib-build: raylib-clean
	$(MAKE) -C $(RAYLIB_SRC_PATH) PLATFORM=PLATFORM_DESKTOP

raylib-clean:
	$(MAKE) -C $(RAYLIB_SRC_PATH) clean

clean:
	rm -rf $(BINDIR)/*.tiny16 $(BINDIR)/tiny16-asm* $(BINDIR)/tiny16-emu* $(BINDIR)/tiny16-tests*

clean-all: clean raylib-clean

compile_commands.json:
	bear -- $(MAKE)
