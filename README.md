# tiny16

A minimalist 16-bit virtual machine with assembler and emulator.

## Overview

tiny16 is a register-based VM featuring:

- 8×8-bit general-purpose registers (R0-R7)
- 64KB address space with memory-mapped I/O
- Simple 3-byte fixed-length instruction format
- Custom ISA with 15 instructions
- Two-pass assembler

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
LOADI R0, 42    ; Load immediate value
LOADI R1, 10    ; Load another value
ADD R0, R1      ; R0 = R0 + R1
HALT            ; Stop execution
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

- **Registers:** R0-R5 (general), R6:R7 (form 16-bit ADDR)
- **Flags:** Z (zero), C (carry)
- **Instructions:** LOADI, LOAD, STORE, ADD, SUB, INC, DEC, AND, OR, XOR, JMP, JZ, JNZ, HALT
- **Memory Layout:**
  - `0x0010-0x1FFF` Code
  - `0x2000-0x3FFF` Data
  - `0x8000-0xBFFF` Stack
  - `0xC000-0xFFFF` Framebuffer & MMIO

## License

See [LICENSE](LICENSE)
