# tiny16

[![CI](https://github.com/rafael84/tiny16/actions/workflows/ci.yml/badge.svg)](https://github.com/rafael84/tiny16/actions/workflows/ci.yml)

A minimalist 16-bit virtual machine with assembler and emulator.

## Features

- 8 general-purpose 8-bit registers (R0-R7)
- 64KB address space with memory-mapped I/O
- 28-instruction ISA with 3-byte fixed-length format
- Assembler with macros and file inclusion
- S-expression high-level language compiler
- 128×128 pixel PPU with tiles, sprites, and scrolling
- 5-channel APU (2 pulse + triangle + noise + wave)
- Gamepad + mouse input support

## Building

**macOS**

```bash
xcode-select --install   # if not already installed
make
```

**Linux (Ubuntu/Debian)**

```bash
sudo apt-get install build-essential libgl1-mesa-dev libx11-dev \
    libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev
make
```

**Windows ([MSYS2](https://www.msys2.org/))**

```bash
pacman -S mingw-w64-x86_64-gcc make
make
```

**WebAssembly (Browser)**

```bash
# Build and run (downloads Emscripten v4.0.23 automatically on first run)
make build-web  # Build for web
make serve-web  # Start local server at http://localhost:8000

# To upgrade or reinstall Emscripten
make clean-emsdk
make build-web
```

Or download pre-built binaries from [Releases](https://github.com/rafael84/tiny16/releases).

## Usage

```bash
build/tiny16-asm examples/asm/demo.asm build/demo.tiny16   # Assemble
build/tiny16-emu build/demo.tiny16                          # Run
```

**Controls:** Arrow keys/WASD (D-pad), Z/X (A/B buttons), Enter (Start), P (Pause), ESC (Exit)

## Examples

**Assembly:**
[`demo.asm`](examples/asm/demo.asm) | [`apu_demo.asm`](examples/asm/apu_demo.asm) | [`input_test.asm`](examples/asm/input_test.asm) | [`scroll_demo.asm`](examples/asm/scroll_demo.asm)

**S-expression:**
[`demo.se`](examples/se/demo.se) | [`apu_demo.se`](examples/se/apu_demo.se) | [`input_test.se`](examples/se/input_test.se) | [`scroll_demo.se`](examples/se/scroll_demo.se)

## Testing

```bash
make tests          # Run all tests
make tests-vm       # VM instruction tests
make tests-asm      # Assembler tests
make tests-sec      # S-expression compiler tests
```

## Editor Support

### Neovim

Syntax highlighting and go-to-definition for `.asm` and `.se` files. See [`misc/nvim/`](misc/nvim/) for installation instructions.

Features:
- Syntax highlighting for assembly and S-expression files
- Jump to definition (`gd`) for functions, constants, and data labels in `.se` files
- Zero dependencies - pure Lua/Vimscript

## Documentation

- [`stdlib/tiny16.inc`](stdlib/tiny16.inc) — Standard library (60+ macros, fully documented)
- [`specs/`](specs/) — ISA and assembler reference
- [`tutorial/`](tutorial/) — 48 step-by-step tutorials

## License

[MIT](LICENSE)
