# tiny16

A minimalist 16-bit virtual machine with assembler and emulator.

## Overview

tiny16 is a register-based VM featuring:

- 8×8-bit general-purpose registers (R0-R7)
- 64KB address space with memory-mapped I/O
- Simple 3-byte fixed-length instruction format
- Custom ISA with 23 instructions
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

- Arrow keys / WASD: D-pad (Up, Down, Left, Right)
- Z/J: A button
- X/K: B button
- Enter: Start
- Space: Select
- P: Pause/unpause
- ESC: Exit

## Quick Examples

**Arithmetic:**

```asm
; Simple arithmetic
LOADI R0, 42    ; Load immediate value
LOADI R1, 10    ; Load another value
ADD R0, R1      ; R0 = R0 + R1 (R0 = 52)
SHL R0          ; R0 = R0 * 2 (R0 = 104)
MOV R2, R0      ; Copy R0 to R2
HALT            ; Stop execution
```

**Interactive programs:**

See `examples/12_input_test.asm` for keyboard control, or try the mouse paint demo:

```bash
bin/tiny16-emu bin/13_mouse_paint.tiny16
```

Controls: Left click to draw, Space to clear.

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

**Input examples:**

```asm
; Read keyboard state
LOADI R6, 0xBF
LOADI R7, 0x00
LOAD  R0             ; R0 = KEYS_STATE
LOADI R1, 0x40       ; bit 6 = Up
AND   R0, R1         ; Check if Up pressed
JZ    NOT_PRESSED    ; Jump if not pressed
; ... handle Up key ...
NOT_PRESSED:

; Read mouse position and button
LOADI R6, 0xBF
LOADI R7, 0x02
LOAD  R0             ; R0 = mouse X (0-127)
LOADI R7, 0x03
LOAD  R1             ; R1 = mouse Y (0-127)
LOADI R7, 0x04
LOAD  R2             ; R2 = mouse buttons
LOADI R3, 0x01       ; bit 0 = left button
AND   R2, R3
JZ    NOT_CLICKED    ; Jump if not clicked
; ... handle mouse click at (R0, R1) ...
NOT_CLICKED:
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
- **Instructions:** LOADI, LOAD, STORE, MOV, ADD, SUB, INC, DEC, AND, OR, XOR, SHL, SHR, PUSH, POP, JMP, JZ, JNZ, JC, JNC, CALL, RET, HALT
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

- **Input:** Keyboard/gamepad state (8 buttons with edge detection), mouse position (0-127), mouse buttons
- **Timers:** Instruction counter (TICK_LOW, TICK_HIGH), frame counter (FRAME_COUNT), VSYNC control
- **Framebuffer:** 128×128 pixel display (RGB332 format)

See `specs/isa.txt` for complete MMIO specification (including planned features).

## License

See [LICENSE](LICENSE)
