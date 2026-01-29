# tiny16

[![CI](https://github.com/rafael84/tiny16/actions/workflows/ci.yml/badge.svg)](https://github.com/rafael84/tiny16/actions/workflows/ci.yml)

A minimalist 16-bit virtual machine with assembler and emulator.

## Features

- 8 general-purpose 8-bit registers (R0-R7)
- 16-bit stack pointer with direct access (MOVSPR/MOVRSP)
- 64KB address space with memory-mapped I/O
- 28-instruction ISA with 3-byte fixed-length format
- Assembler with macros and file inclusion
- 128×128 pixel framebuffer (RGB332)
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
bin/tiny16-asm examples/demo.asm bin/demo.tiny16   # Assemble
bin/tiny16-emu bin/demo.tiny16                      # Run
```

**Controls:** Arrow keys/WASD (D-pad), Z/X (A/B buttons), Enter (Start), P (Pause), ESC (Exit)

## Examples

**Visual Demos:**
- [`demo.asm`](examples/demo.asm) — 32 bouncing sprites with PPU
- [`boing_ball.asm`](examples/boing_ball.asm) — Bouncing ball animation
- [`input_test.asm`](examples/input_test.asm) — Keyboard-controlled sprite
- [`mouse_paint.asm`](examples/mouse_paint.asm) — Drawing with mouse

## Documentation

- [`stdlib/tiny16.inc`](stdlib/tiny16.inc) — Standard library (60+ macros, fully documented)
- [`specs/`](specs/) — ISA and assembler reference
- [`tutorial/`](tutorial/) — 49 step-by-step tutorials

## License

[MIT](LICENSE)
