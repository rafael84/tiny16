# tiny16

A minimalist 16-bit virtual machine with assembler and emulator.

## Overview

tiny16 is a register-based VM featuring:

- 8×8-bit general-purpose registers (R0-R7)
- 64KB address space with memory-mapped I/O
- Simple 3-byte fixed-length instruction format
- Custom ISA with 21 instructions
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

## Quick Example

```asm
; Simple arithmetic
LOADI R0, 42    ; Load immediate value
LOADI R1, 10    ; Load another value
ADD R0, R1      ; R0 = R0 + R1 (R0 = 52)
SHL R0          ; R0 = R0 * 2 (R0 = 104)
MOV R2, R0      ; Copy R0 to R2
HALT            ; Stop execution
```

**Subroutine example:**

```asm
START:
    LOADI R0, 5
    LOADI R1, 3
    CALL  ADD_NUMBERS    ; Call subroutine
    HALT

ADD_NUMBERS:
    ADD   R0, R1         ; R0 = R0 + R1
    RET                  ; Return to caller
```

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

- **Registers:** R0-R5 (general purpose), R6:R7 (form 16-bit ADDR for LOAD/STORE)
- **Flags:** Z (zero), C (carry)
- **Instructions:** LOADI, LOAD, STORE, MOV, ADD, SUB, INC, DEC, AND, OR, XOR, SHL, SHR, PUSH, POP, JMP, JZ, JNZ, CALL, RET, HALT
- **Memory Layout:**
  - `0x0000-0x000F` File signature (magic, version, entrypoint)
  - `0x0010-0x1FFF` Code (8176 bytes)
  - `0x2000-0x3FFF` Data (8192 bytes)
  - `0x4000-0x7FFF` Extended/reserved (16KB)
  - `0x8000-0xBEFF` Stack (grows down, ~16KB)
  - `0xBF00-0xBFFF` MMIO control registers (256 bytes)
  - `0xC000-0xFFFF` Framebuffer (128×128 pixels, RGB332)

## MMIO Features

Currently implemented memory-mapped I/O:

- **Timers:** Instruction counter (TICK_LOW, TICK_HIGH), frame counter (FRAME_COUNT), VSYNC control
- **Framebuffer:** 128×128 pixel display (RGB332 format)

See `specs/isa.txt` for complete MMIO specification (including planned features).

## License

See [LICENSE](LICENSE)
