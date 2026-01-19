.PHONY: all asm tests examples emulator emulator-web build-web serve-web clean clean-all clean-emsdk raylib-build raylib-clean raylib-build-web

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
    RAYLIB_LDFLAGS = $(RAYLIB_LIB_NATIVE) \
        -lGL -lm -lpthread -ldl -lrt -lX11
endif

ifeq ($(PLATFORM),windows)
    CC = gcc
    EXE_EXT = .exe
    RAYLIB_LDFLAGS = $(RAYLIB_LIB_NATIVE) \
        -static -lopengl32 -lgdi32 -lwinmm -lshell32
endif

CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Ivm -Ithirdparty
RAYLIB_INCLUDE = -I$(RAYLIB_SRC_PATH)

# Emscripten settings for web build
EMCC = emcc
EMCC_FLAGS = -std=c99 -Wall -Ivm -Ithirdparty $(RAYLIB_INCLUDE) \
    -s USE_GLFW=3 -s ASYNCIFY \
    -s INITIAL_MEMORY=134217728 \
    -s STACK_SIZE=2097152 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s EXPORTED_FUNCTIONS='["_main","_tiny16_web_reload_program"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
    -DPLATFORM_WEB \
    --shell-file $(WEBDIR)/shell.html

BINDIR = bin
WEBDIR = emulator/web

all: tests asm emulator examples

$(BINDIR):
	mkdir -p $@

tests: $(BINDIR) vm/*.c vm/*.h tests/*.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/tiny16-tests$(EXE_EXT) tests/tiny16.c
	$(BINDIR)/tiny16-tests$(EXE_EXT) | column -t | paste - - -

asm: $(BINDIR) vm/*.c vm/*.h asm/*.h asm/*.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/tiny16-asm$(EXE_EXT) asm/tiny16.c

emulator: $(BINDIR) vm/*.c vm/*.h emulator/*.c $(RAYLIB_LIB_NATIVE) | $(BINDIR)
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -o $(BINDIR)/tiny16-emu$(EXE_EXT) \
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

EXAMPLES := $(wildcard examples/*.asm)
examples: asm
	@for F in $(EXAMPLES); do \
		$(BINDIR)/tiny16-asm$(EXE_EXT) $$F $(BINDIR)/$$(basename $$F .asm).tiny16; \
	done

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
	rm -rf $(BINDIR)/*.tiny16 $(BINDIR)/tiny16-asm* $(BINDIR)/tiny16-emu* $(BINDIR)/tiny16-tests*

clean-all: clean raylib-clean

clean-emsdk:
	rm -rf $(EMSDK_PATH)

compile_commands.json:
	bear -- $(MAKE)
