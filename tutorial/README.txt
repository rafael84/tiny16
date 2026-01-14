Tiny16 Assembly Tutorial - Structured Learning Path
====================================================

A comprehensive, progressive course in tiny16 assembly programming,
covering fundamental assembly concepts and tiny16-specific features.


QUICK START
-----------

    cd tutorial/
    vim 01_load_and_halt.asm
    ../bin/tiny16-asm 01_load_and_halt.asm 01_load_and_halt.tiny16
    ../bin/tiny16-emu 01_load_and_halt.tiny16 -m 100 -d


EMULATOR FLAGS
--------------

    -m <n>    Run max N instructions (headless, no GUI)
    -d        Dump CPU state at end
    -df       Dump CPU + framebuffer
    -t        Trace every instruction
    -tm       Trace only memory operations

    Combine: -m 100 -d for quick testing


LEARNING PATH
=============

Level 1: Fundamentals (01-08)
------------------------------
Master the basics: registers, loading values, simple arithmetic.

    01  Load and Halt         LOADI, HALT            R0 = 42
    02  Add Two Numbers        ADD                    R0 = 15
    03  Copy Register          MOV                    R1 = R0
    04  Simple Subtraction     SUB                    R0 = 5
    05  Increment/Decrement    INC, DEC               R0 = 11
    06  Zero a Register        XOR                    R0 = 0
    07  Double a Number        SHL or ADD             R0 = 20
    08  Halve a Number         SHR                    R0 = 5

Concepts: Basic instructions, register operations, flags (Z flag)


Level 2: Control Flow (09-16)
------------------------------
Learn branching, loops, and conditional execution.

    09  Unconditional Jump     JMP                    Infinite loop
    10  Simple Loop            DEC, JNZ               Count down
    11  Count Up               INC, CMP, JZ           Count to 10
    12  Compare Numbers        CMP, JZ, JC            R0 = max(5,8)
    13  If-Else Logic          CMP, JC, JMP           Branching
    14  For Loop               Loop N times           Sum 1..N
    15  While Loop             Condition at start     Find first > 100
    16  Nested Loops           Loop in loop           2D counter

Concepts: Labels, jumps, CMP instruction, Z and C flags, loop patterns


Level 3: Memory & Data Structures (17-24)
------------------------------------------
Work with memory, arrays, and data sections.

    17  Load from Memory       LOAD [PAIR]            Read byte
    18  Store to Memory        STORE [PAIR]           Write byte
    19  Array Access           LOAD with offset       Read array[2]
    20  Post-Increment         LOAD [PAIR]+           Array traversal
    21  Post-Decrement         LOAD [PAIR]-           Reverse traversal
    22  Offset Addressing      LOAD [PAIR + N]        Struct access
    23  Array Sum              Loop + LOAD            Sum array
    24  Array Copy             LOAD + STORE           Copy 10 bytes

Concepts: Register pairs (R0:R1, R2:R3, R4:R5, R6:R7), addressing modes,
          .data section, DB directive, arrays


Level 4: Stack & Subroutines (25-32)
-------------------------------------
Master the stack and learn structured programming.

    25  Push and Pop           PUSH, POP              Stack basics
    26  Preserve Registers     PUSH/POP in loop       Save/restore
    27  Simple Subroutine      CALL, RET              Function call
    28  Subroutine with Args   Pass via registers     Add(a, b)
    29  Nested Calls           CALL in CALL           Call chain
    30  Recursive Countdown    Recursion              Countdown(n)
    31  Recursive Factorial    Recursion + stack      Factorial(n)
    32  Stack Frame            SP manipulation        Local variables

Concepts: Stack pointer (SP), PUSH/POP, CALL/RET, calling conventions,
          recursion, stack frames, register preservation


Level 5: Advanced Arithmetic (33-38)
-------------------------------------
Multi-byte operations and efficient algorithms.

    33  16-bit Addition        ADC                    Add two words
    34  16-bit Subtraction     SBC                    Subtract words
    35  8-bit Multiply         Repeated addition      a × b
    36  Fast Multiply          Shift and add          a × b
    37  8-bit Divide           Repeated subtraction   a ÷ b
    38  Modulo Operation       Division remainder     a % b

Concepts: Carry flag (C), ADC/SBC for multi-byte math, multiplication
          and division algorithms, optimization techniques


Level 6: Bit Manipulation (39-44)
----------------------------------
Master bitwise operations and low-level techniques.

    39  Set/Clear Bits         OR, AND                Bit manipulation
    40  Test Bit               AND, JZ                Check if set
    41  Toggle Bits            XOR                    Flip bits
    42  Count Set Bits         Shift + test           Population count
    43  Rotate Left            Manual rotation        Circular shift
    44  Byte Swap              Nibble manipulation    Swap high/low

Concepts: Bit masking, shift operations (SHL, SHR), bitwise logic,
          bit-level algorithms, binary representation


Level 7: MMIO & Hardware (45-49)
---------------------------------
Interact with hardware through memory-mapped I/O.

    45  Read Input State       MMIO read              Get key state
    46  Button Press Detection Edge detection         Single press
    47  Read Mouse Position    MMIO multiple reads    X, Y coords
    48  Draw Single Pixel      Framebuffer write      Plot (x, y)
    49  Draw Horizontal Line   FB loop                10-pixel line

Concepts: Memory-mapped I/O (MMIO), hardware registers, input devices
          (keyboard, mouse), framebuffer, RGB332 color format


Level 8: Advanced Topics (50+)
-------------------------------
Optional challenge exercises for mastery.

    50  TIMES Directive        Repeat instructions    Efficient code
    51  Multi-Section Program  .code + .data          Structured code
    52  String Operations      String copying         Text handling
    53  Nested Data Structures Struct-like access     Complex data

Concepts: TIMES pseudo-instruction, section organization, real-world patterns


COMMON PATTERNS
===============

Basic Loop (count down from N)
-------------------------------
    LOADI R0, 10
loop:
    ; ... loop body ...
    DEC R0
    JNZ loop

Count Up Loop
-------------
    LOADI R0, 0      ; counter
    LOADI R1, 10     ; limit
loop:
    ; ... loop body ...
    INC R0
    CMP R0, R1
    JNZ loop

Memory Access (Direct)
----------------------
    LOADI R6, 0x40   ; address high byte
    LOADI R7, 0x00   ; address low byte (0x4000)
    LOAD  R0, [R6:R7]     ; read byte at 0x4000
    STORE R1, [R6:R7]     ; write byte to 0x4000

Array Traversal (Post-Increment)
---------------------------------
    LOADI R6, 0x40   ; array base high
    LOADI R7, 0x00   ; array base low
    LOADI R0, 5      ; count
loop:
    LOAD  R1, [R6:R7]+    ; read and auto-increment
    ; ... process R1 ...
    DEC   R0
    JNZ   loop

Subroutine Call Pattern
-----------------------
    LOADI R0, 5      ; argument in R0
    CALL  my_func    ; call subroutine
    ; ... result in R0 ...
    HALT

my_func:
    PUSH  R1         ; preserve registers
    ; ... function body ...
    POP   R1         ; restore registers
    RET              ; return to caller

Compare and Branch
------------------
    LOADI R0, 10
    LOADI R1, 20
    CMP   R0, R1     ; compare R0 with R1
    JZ    equal      ; if R0 == R1
    JC    less       ; if R0 < R1
    ; else R0 > R1
greater:
    ; ...
    JMP   done
less:
    ; ...
    JMP   done
equal:
    ; ...
done:


REGISTER CONVENTIONS
====================

While tiny16 has no enforced conventions, these suggestions help:

R0, R1   : Return values, temporary calculations
R2, R3   : Arguments, temporary storage
R4, R5   : Loop counters, saved values
R6, R7   : Memory addressing (default pair for LOAD/STORE)

Register Pairs for Addressing:
- R0:R1  (pair 0)
- R2:R3  (pair 1)
- R4:R5  (pair 2)
- R6:R7  (pair 3) ← conventional default


FLAGS REFERENCE
===============

Z (Zero Flag)
- Set (Z=1) when result is 0
- Clear (Z=0) when result is non-zero
- Updated by: ADD, SUB, ADC, SBC, INC, DEC, AND, OR, XOR, CMP, SHL, SHR

C (Carry Flag)
- Set (C=1) on overflow (ADD, ADC) or borrow (SUB, SBC, CMP)
- Set (C=1) when bit shifted out (SHL, SHR)
- Cleared (C=0) by INC, DEC, AND, OR, XOR
- Used for: multi-byte arithmetic, unsigned comparisons

CMP Instruction:
    CMP R0, R1   ; compute R0 - R1, set flags (don't store result)
    JZ  equal    ; R0 == R1
    JC  less     ; R0 < R1 (unsigned)
    JNC greater  ; R0 >= R1 (unsigned)


MEMORY MAP
==========

0x0000-0x000F : File signature (magic, version, entry)
0x0010-0x3FFF : Code segment (~16KB)
0x4000-0x4FFF : Data segment (4KB)
0x5000-0x6FFF : Tile data (8KB, for graphics)
0x7000-0x791F : Graphics memory (tilemap, sprites, palette)
0x8000-0xBEFF : Stack (grows down, ~16KB)
0xBF00-0xBFFF : MMIO control registers (256 bytes)
0xC000-0xFFFF : Framebuffer (128×128 pixels)


MMIO ADDRESSES
==============

Input Devices
-------------
0xBF00 : KEYS_STATE      Keyboard/gamepad state (8 bits)
                         bit 7=Down, 6=Up, 5=Left, 4=Right
                         bit 3=B, 2=A, 1=Start, 0=Select
0xBF01 : KEYS_PRESSED    Edge detection (cleared on read)
                         Use for single-press actions
0xBF02 : MOUSE_X         Mouse X position (0-127)
0xBF03 : MOUSE_Y         Mouse Y position (0-127)
0xBF04 : MOUSE_BUTTONS   bit 0=left, 1=right, 2=middle

Timers
------
0xBF20 : TICK_LOW        CPU instruction counter (low byte)
0xBF21 : TICK_HIGH       CPU instruction counter (high byte)
0xBF22 : FRAME_COUNT     Frame counter (60Hz, wraps at 256)

PPU (Graphics)
--------------
0xBF30 : PPU_CTRL        bit 0=enable BG, 1=enable sprites
                         bit 7=RENDER_NOW (trigger render)
0xBF31 : PPU_SCROLL_X    Background scroll X (0-255)
0xBF32 : PPU_SCROLL_Y    Background scroll Y (0-255)
0xBF33 : PPU_STATUS      bit 0=VBLANK (cleared on read)

Framebuffer
-----------
0xC000-0xFFFF : 128×128 pixel display
                Address = 0xC000 + (y << 7) + x
                Format: RGB332 (1 byte per pixel)
                        RRRGGGBB
                0x00 = black, 0xFF = white
                0xE0 = red, 0x1C = green, 0x03 = blue


ASSEMBLER FEATURES
==================

Sections
--------
    section .code    ; instructions go here (starts at 0x0010)
    section .data    ; data definitions (starts at 0x4000)

Data Definitions
----------------
    DB (Define Byte)
        msg: DB "Hello\0"
        values: DB 1, 2, 3, 0xFF
        mixed: DB "A", 65, 0x41

TIMES (Repeat)
--------------
    TIMES count instruction

    Code examples:
        TIMES 4 SHR R0        ; divide by 16
        TIMES 3 INC R1        ; add 3

    Data examples:
        buffer: TIMES 256 DB 0         ; 256 zero bytes
        pattern: TIMES 10 DB 0xAA      ; 10 bytes of 0xAA

Comments
--------
    ; This is a comment (semicolon to end of line)

Labels
------
    loop:           ; label marks address
        INC R0
        JMP loop    ; jump to label

Numbers
-------
    Decimal:  42
    Hex:      0x2A
    Binary:   0b00101010


TIPS & TRICKS
=============

1. Always end programs with HALT
2. Use -m 100 -d for quick testing without GUI
3. Use -t to trace instruction execution
4. XOR R, R is fastest way to zero a register
5. SHL R is faster than ADD R, R (both double)
6. Use CMP instead of SUB when you need to preserve values
7. INC/DEC clear the carry flag
8. Post-increment addressing ([R6:R7]+) simplifies array loops
9. Save/restore registers in subroutines with PUSH/POP
10. Stack grows down from 0xBEFF


DEBUGGING
=========

Common Errors:
- Missing HALT: program runs into garbage memory
- Wrong register pair: LOAD uses R6:R7 by default, not R0:R1
- Infinite loops: use -m flag to limit instructions
- Stack overflow: too many PUSH or nested CALLs
- Off-by-one: check loop conditions carefully

Tools:
- Use -d to dump registers after execution
- Use -t to trace every instruction
- Use -tm to trace only memory operations
- Check that PC is at expected address


RESOURCES
=========

Specification:
    ../specs/isa.txt           Complete instruction set reference
    ../specs/assembler.txt     Assembler syntax and directives

Examples:
    ../examples/*.asm          Working example programs

Solutions:
    SOLUTIONS.txt              Solutions to all exercises


ADVANCED FEATURES NOT COVERED IN BASIC EXERCISES
=================================================

JC/JNC Instructions:
- JC (Jump if Carry): Useful for detecting overflow in addition
- JNC (Jump if No Carry): Useful for unsigned comparisons (>=)
- Example use cases: multi-byte arithmetic, unsigned compare logic

Post-Decrement Addressing [PAIR]-:
- Access memory then decrement address
- Useful for reverse traversal, stack-like operations

TIMES with Data:
- Allocate large buffers: TIMES 256 DB 0
- Initialize patterns: TIMES 10 DB 0xAA, 0x55
- See exercises 50-53 for advanced usage


CHALLENGE YOURSELF
==================

After completing all exercises, try these:

- Implement quicksort on an array
- Create a simple calculator (add/sub/mul/div)
- Build a sprite animation system
- Write a text scrolling routine
- Design a simple game (snake, pong, etc.)
- Implement a pseudo-random number generator
- Create a drawing program with mouse
- Build a music player with timer-based notes


Happy coding! Start with tutorial 01 and work your way up.
Remember: every expert was once a beginner. 🚀
