# tiny16

A minimalist 16-bit virtual machine with assembler and emulator.

## Overview

tiny16 is a register-based VM featuring:

- 8×8-bit general-purpose registers (R0-R7)
- 64KB address space with memory-mapped I/O
- Simple 3-byte fixed-length instruction format
- Custom ISA with 26 instructions
- Two-pass assembler
- Subroutine support with CALL/RET

## Building

```bash
make          # Build everything and run tests
make asm      # Build assembler only
make emulator # Build emulator only
make examples # Assemble all examples
```

## Usage

**Assemble a program:**

```bash
bin/tiny16-asm examples/demo.asm bin/demo.tiny16
```

**Run the bytecode:**

```bash
bin/tiny16-emu bin/demo.tiny16
```

**Emulator controls:**

| Key               | Action                        |
| ----------------- | ----------------------------- |
| Arrow keys / WASD | D-pad (Up, Down, Left, Right) |
| Z / J             | A button                      |
| X / K             | B button                      |
| Enter             | Start                         |
| Space             | Select                        |
| P                 | Pause/unpause                 |
| ESC               | Exit                          |

## Quick Example

```asm
; Add two numbers and multiply by 2
LOADI R0, 42        ; Load immediate value
LOADI R1, 10        ; Load another value
ADD   R0, R1        ; R0 = R0 + R1 (R0 = 52)
SHL   R0            ; R0 = R0 * 2 (R0 = 104)
MOV   R2, R0        ; Copy R0 to R2
HALT                ; Stop execution
```

For more examples including subroutines, comparisons, input handling, and multi-byte arithmetic, see:

- `examples/` directory for complete programs
- `specs/isa.txt` for instruction examples

## Project Structure

```
├── vm/           # VM core (CPU, memory)
├── asm/          # Assembler implementation
├── emulator/     # Emulator/runner
├── specs/        # ISA and assembler specs
├── examples/     # Example programs
└── tests/        # Test suite
```

## ISA Highlights

**Registers:**

| Register | Purpose                                   |
| -------- | ----------------------------------------- |
| R0-R7    | General purpose (8-bit)                   |
| R0:R1    | Register pair for addressing              |
| R2:R3    | Register pair for addressing              |
| R4:R5    | Register pair for addressing              |
| R6:R7    | Register pair for addressing (convention) |

**Flags:**

| Flag | Description       |
| ---- | ----------------- |
| Z    | Zero flag         |
| C    | Carry/borrow flag |

**Instructions (26 total):**

LOADI, LOAD, STORE, MOV, ADD, SUB, ADC, SBC, INC, DEC, AND, OR, XOR, CMP, SHL, SHR, PUSH, POP, JMP, JZ, JNZ, JC, JNC, CALL, RET, HALT

**Memory Layout:**

| Address Range   | Size        | Purpose                                     |
| --------------- | ----------- | ------------------------------------------- |
| `0x0000-0x000F` | 16 bytes    | File signature (magic, version, entrypoint) |
| `0x0010-0x3FFF` | ~16 KB      | Code segment                                |
| `0x4000-0x4FFF` | 4 KB        | Data segment                                |
| `0x5000-0x6FFF` | 8 KB        | Tiles (256 × 32 bytes)                      |
| `0x7000-0x73FF` | 1 KB        | Tilemap (32 × 32)                           |
| `0x7400-0x77FF` | 1 KB        | Attrmap (32 × 32)                           |
| `0x7800-0x78FF` | 256 bytes   | OAM (64 sprites × 4 bytes)                  |
| `0x7900-0x791F` | 32 bytes    | Palette (16 colors × 2 bytes)               |
| `0x8000-0xBEFF` | ~16 KB      | Stack (grows down)                          |
| `0xBF00-0xBFFF` | 256 bytes   | MMIO control registers                      |
| `0xC000-0xFFFF` | 16384 bytes | Framebuffer (128×128 pixels, RGB332)        |

## MMIO Features

Currently implemented memory-mapped I/O:

- **Input:** Keyboard/gamepad state (8 buttons with edge detection), mouse position (0-127), mouse buttons
- **Timers:** Instruction counter (TICK_LOW, TICK_HIGH), frame counter (FRAME_COUNT), VSYNC control
- **Framebuffer:** 128×128 pixel display (RGB332 format)

See `specs/isa.txt` for complete MMIO specification (including planned features).

## License

See [LICENSE](LICENSE)
