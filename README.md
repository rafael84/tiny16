# tiny16

[![CI](https://github.com/rafael84/tiny16/actions/workflows/ci.yml/badge.svg)](https://github.com/rafael84/tiny16/actions/workflows/ci.yml)

A minimalist 16-bit virtual machine with assembler and emulator.

## Features

- 8 general-purpose 8-bit registers (R0-R7)
- 64KB address space with memory-mapped I/O
- 26-instruction ISA with 3-byte fixed-length format
- 128×128 pixel framebuffer (RGB332)
- Gamepad + mouse input support

## Building

Requires: C compiler (gcc/clang), raylib dependencies

```bash
# Linux: sudo apt-get install libgl1-mesa-dev libx11-dev libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev
# macOS: Xcode command line tools

make              # Build everything
```

## Usage

```bash
bin/tiny16-asm examples/demo.asm bin/demo.tiny16   # Assemble
bin/tiny16-emu bin/demo.tiny16                      # Run
```

**Controls:** Arrow keys/WASD (D-pad), Z/X (A/B buttons), Enter (Start), P (Pause), ESC (Exit)

## Example

```asm
LOADI R0, 42        ; Load immediate
LOADI R1, 10
ADD   R0, R1        ; R0 = 52
SHL   R0            ; R0 = 104
HALT
```

## Documentation

- [`specs/isa.txt`](specs/isa.txt) — Instruction set reference
- [`specs/assembler.txt`](specs/assembler.txt) — Assembler syntax
- [`examples/`](examples/) — Example programs
- [`tutorial/`](tutorial/) — Step-by-step tutorials

## License

[MIT](LICENSE)
