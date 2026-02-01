.PHONY: all asm sec tests tests-vm tests-asm tests-sec examples examples-asm examples-se examples-assets emulator emulator-web build-web serve-web
.PHONY: tools clean clean-all clean-emsdk raylib-build raylib-clean raylib-build-web format ci

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
RAYLIB_LIB_NATIVE = $(RAYLIB_SRC_PATH)/libraylib_native.a
RAYLIB_LIB_WEB = $(RAYLIB_SRC_PATH)/libraylib_web.a

EMSDK_VERSION = 4.0.23
EMSDK_PATH = thirdparty/emsdk
EMSDK_ENV = $(EMSDK_PATH)/emsdk_env.sh
EMSDK_URL = https://github.com/emscripten-core/emsdk/archive/refs/tags/$(EMSDK_VERSION).zip

ifeq ($(PLATFORM),macos)
    CC = clang
    EXE_EXT =
    TINY16_LDFLAGS =
    RAYLIB_LDFLAGS = $(RAYLIB_LIB_NATIVE) \
        -framework OpenGL \
        -framework Cocoa \
        -framework IOKit \
        -framework CoreAudio \
        -framework CoreVideo
endif

ifeq ($(PLATFORM),linux)
    CC = gcc
    EXE_EXT =
    TINY16_LDFLAGS =
    RAYLIB_LDFLAGS = $(RAYLIB_LIB_NATIVE) \
        -lGL -lm -lpthread -ldl -lrt -lX11
endif

ifeq ($(PLATFORM),windows)
    CC = gcc
    EXE_EXT = .exe
    # Windows binaries get a small default stack; increase it to avoid
    # crashing with STATUS_STACK_OVERFLOW (0xC00000FD) when assembling large files.
    TINY16_LDFLAGS = -Wl,--stack,8388608
    RAYLIB_LDFLAGS = $(RAYLIB_LIB_NATIVE) \
        -static -lopengl32 -lgdi32 -lwinmm -lshell32
endif

CFLAGS = -std=c99 -O3 -Wall -Wextra -Wpedantic -Ivm -Ithirdparty
RAYLIB_INCLUDE = -I$(RAYLIB_SRC_PATH)

# Emscripten settings for web build
EMCC = emcc
EMCC_FLAGS = -std=c99 -O3 -Wall -DNDEBUG -Ivm -Ithirdparty $(RAYLIB_INCLUDE) \
    -s USE_GLFW=3 -s ASYNCIFY \
    -s INITIAL_MEMORY=134217728 \
    -s STACK_SIZE=2097152 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s EXPORTED_FUNCTIONS='["_main","_tiny16_web_reload_program"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","HEAPF32"]' \
    -DPLATFORM_WEB \
    --shell-file $(WEBDIR)/shell.html

BUILDDIR = build
WEBDIR = emulator/web
TOOLSDIR = tools

all: format tests asm sec emulator examples tools

ci: tests asm sec emulator examples tools

$(BUILDDIR):
	mkdir -p $@

tests: tests-vm tests-asm tests-sec

tests-vm: $(BUILDDIR) vm/*.c vm/*.h tests/*.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(TINY16_LDFLAGS) -o $(BUILDDIR)/tiny16-vm-tests$(EXE_EXT) tests/vm_test.c
	$(BUILDDIR)/tiny16-vm-tests$(EXE_EXT)

tests-asm: $(BUILDDIR) vm/*.c vm/*.h asm/*.c asm/*.h tests/*.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(TINY16_LDFLAGS) -o $(BUILDDIR)/tiny16-asm-tests$(EXE_EXT) tests/asm_test.c
	$(BUILDDIR)/tiny16-asm-tests$(EXE_EXT)

tests-sec: $(BUILDDIR) sec/*.c sec/*.h tests/*.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(TINY16_LDFLAGS) -Isec -o $(BUILDDIR)/tiny16-sec-tests$(EXE_EXT) tests/sec_test.c
	$(BUILDDIR)/tiny16-sec-tests$(EXE_EXT)

asm: $(BUILDDIR) vm/*.c vm/*.h asm/*.h asm/*.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(TINY16_LDFLAGS) -o $(BUILDDIR)/tiny16-asm$(EXE_EXT) asm/tiny16.c

sec: $(BUILDDIR) sec/*.h sec/*.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(TINY16_LDFLAGS) -Isec -o $(BUILDDIR)/tiny16-sec$(EXE_EXT) sec/tiny16se.c

tools: $(BUILDDIR) $(TOOLSDIR)/*.c vm/*.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(TINY16_LDFLAGS) -o $(BUILDDIR)/png2tiles$(EXE_EXT) $(TOOLSDIR)/png2tiles.c -lm

emulator: $(BUILDDIR) vm/*.c vm/*.h emulator/*.c $(RAYLIB_LIB_NATIVE) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(TINY16_LDFLAGS) $(RAYLIB_INCLUDE) -o $(BUILDDIR)/tiny16-emu$(EXE_EXT) \
		emulator/tiny16_core.c emulator/tiny16_native.c $(RAYLIB_LDFLAGS)

emulator-web: $(EMSDK_ENV) $(RAYLIB_LIB_WEB) $(WEBDIR) vm/*.c vm/*.h emulator/*.c | $(WEBDIR)
	@echo "Building emulator for web..."
	@bash -c "source $(EMSDK_ENV) && $(EMCC) $(EMCC_FLAGS) -o $(WEBDIR)/tiny16-emu.html \
		emulator/tiny16_core.c emulator/tiny16_web.c $(RAYLIB_LIB_WEB)"

build-web: emulator-web examples
	@echo ""
	@echo "Web build complete! To run:"
	@echo "  make serve-web"
	@echo "Then open: http://localhost:8000/tiny16-emu.html"

serve-web: $(WEBDIR)
	@if [ ! -f "$(WEBDIR)/tiny16-emu.html" ]; then \
		echo "Error: Web build not found. Run 'make build-web' first."; \
		exit 1; \
	fi
	@echo "Starting web server at http://localhost:8000"
	@echo "Open: http://localhost:8000/tiny16-emu.html"
	@echo "Press Ctrl+C to stop"
	@python3 -m http.server 8000 --directory $(WEBDIR)

# Download and extract emsdk if not present
$(EMSDK_PATH)/emsdk:
	@echo "Downloading Emscripten SDK $(EMSDK_VERSION)..."
	@mkdir -p thirdparty
	@if command -v wget > /dev/null; then \
		wget -q --show-progress -O thirdparty/emsdk.zip $(EMSDK_URL) || { rm -f thirdparty/emsdk.zip; exit 1; }; \
	elif command -v curl > /dev/null; then \
		curl -L -o thirdparty/emsdk.zip $(EMSDK_URL) || { rm -f thirdparty/emsdk.zip; exit 1; }; \
	else \
		echo "Error: Neither wget nor curl found"; \
		exit 1; \
	fi
	@echo "Extracting..."
	@unzip -q thirdparty/emsdk.zip -d thirdparty || { rm -f thirdparty/emsdk.zip; exit 1; }
	@mv thirdparty/emsdk-$(EMSDK_VERSION) $(EMSDK_PATH)
	@rm -f thirdparty/emsdk.zip

# Install and activate emsdk toolchain
$(EMSDK_ENV): $(EMSDK_PATH)/emsdk
	@echo "Installing Emscripten toolchain..."
	$(EMSDK_PATH)/emsdk install latest
	$(EMSDK_PATH)/emsdk activate latest

$(WEBDIR):
	mkdir -p $@

EXAMPLES_ASM := $(wildcard examples/asm/*.asm)
EXAMPLES_ASM_OUTPUTS := $(patsubst examples/asm/%.asm,$(BUILDDIR)/%.tiny16,$(EXAMPLES_ASM))

EXAMPLES_SE := $(wildcard examples/se/*.se)
EXAMPLES_SE_OUTPUTS := $(patsubst examples/se/%.se,$(BUILDDIR)/%_se.tiny16,$(EXAMPLES_SE))

EXAMPLES_ASSETS := $(wildcard examples/assets/*.png)
EXAMPLES_ASSETS_OUTPUTS := $(patsubst examples/assets/%.png,examples/includes/%.inc,$(EXAMPLES_ASSETS))

EXAMPLE_INCLUDES := $(wildcard stdlib/*.inc) $(wildcard examples/includes/*.inc) $(wildcard tutorial/includes/*.inc)

examples: examples-asm examples-se

examples-asm: asm $(EXAMPLES_ASM_OUTPUTS)

examples-se: asm sec $(EXAMPLES_SE_OUTPUTS)

# ASM examples: .asm -> .tiny16
$(BUILDDIR)/%.tiny16: examples/asm/%.asm $(EXAMPLE_INCLUDES) | $(BUILDDIR)
	$(BUILDDIR)/tiny16-asm$(EXE_EXT) $< $@

# SE examples: .se -> .asm -> .tiny16
$(BUILDDIR)/%_se.tiny16: examples/se/%.se $(EXAMPLE_INCLUDES) | $(BUILDDIR)
	$(BUILDDIR)/tiny16-sec$(EXE_EXT) $< $(BUILDDIR)/$*_se.asm
	$(BUILDDIR)/tiny16-asm$(EXE_EXT) $(BUILDDIR)/$*_se.asm $@

# PNG -> INC: convert PNG assets to assembly include files
examples-assets: tools $(EXAMPLES_ASSETS_OUTPUTS)

examples/includes/%.inc: examples/assets/%.png | tools
	$(BUILDDIR)/png2tiles$(EXE_EXT) $< $@

$(RAYLIB_LIB_NATIVE):
	@echo "Building raylib for native..."
	@if [ -f "$(RAYLIB_LIB_WEB)" ]; then \
		echo "Cleaning raylib (switching from web to native)..."; \
		$(MAKE) -C $(RAYLIB_SRC_PATH) clean -s; \
	fi
	@$(MAKE) -C $(RAYLIB_SRC_PATH) PLATFORM=PLATFORM_DESKTOP -s
	@mv $(RAYLIB_SRC_PATH)/libraylib.a $(RAYLIB_LIB_NATIVE)

$(RAYLIB_LIB_WEB): $(EMSDK_ENV)
	@echo "Building raylib for web..."
	@if [ -f "$(RAYLIB_LIB_NATIVE)" ]; then \
		echo "Cleaning raylib (switching from native to web)..."; \
		$(MAKE) -C $(RAYLIB_SRC_PATH) clean -s; \
	fi
	@bash -c "source $(EMSDK_ENV) && $(MAKE) -C $(RAYLIB_SRC_PATH) PLATFORM=PLATFORM_WEB -s"
	@mv $(RAYLIB_SRC_PATH)/libraylib.a $(RAYLIB_LIB_WEB)

raylib-build: raylib-clean $(RAYLIB_LIB_NATIVE)

raylib-build-web: raylib-clean $(RAYLIB_LIB_WEB)

raylib-clean:
	$(MAKE) -C $(RAYLIB_SRC_PATH) clean
	@rm -f $(RAYLIB_LIB_NATIVE) $(RAYLIB_LIB_WEB)

clean:
	rm -rf $(BUILDDIR)/*.tiny16 $(BUILDDIR)/*_se.asm $(BUILDDIR)/tiny16-asm* $(BUILDDIR)/tiny16-sec* $(BUILDDIR)/tiny16-emu* $(BUILDDIR)/tiny16-vm-tests* $(BUILDDIR)/tiny16-asm-tests* $(BUILDDIR)/tiny16-sec-tests*

clean-all: clean raylib-clean

clean-emsdk:
	rm -rf $(EMSDK_PATH)

format:
	find . -path ./thirdparty -prune -o \( -name '*.c' -o -name '*.h' \) -print | xargs clang-format -i

compile_commands.json:
	bear -- $(MAKE)
