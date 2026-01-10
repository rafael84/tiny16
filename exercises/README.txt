Tiny16 Assembly Exercises
==========================

Progressive exercises for mastering tiny16 assembly.


QUICK START
-----------

    nano 01_double_number.asm
    ../bin/tiny16-asm 01_double_number.asm 01_double_number.tiny16
    ../bin/tiny16-emu 01_double_number.tiny16 -m 100 -d


EMULATOR FLAGS
--------------

    -m <n>    Run max N instructions (headless, no GUI)
    -d        Dump CPU state at end
    -df       Dump CPU + framebuffer
    -t        Trace instructions

    Combine: -m 100 -d for quick testing


EXERCISES
---------

Level 1: Basic Operations
    01  Double a Number        LOADI, ADD     R0 = 10
    02  Triple a Number        ADD, MOV       R0 = 21
    03  Compute 2^3            SHL            R0 = 8
    04  Swap Registers         MOV            R0 = 20, R1 = 10
    05  Clear Register         XOR            R0 = 0

Level 2: Loops & Conditionals
    06  Count to 10            INC, JNZ       R0 = 10
    07  Sum N Numbers          Loops          R0 = 15
    08  Find Maximum           SUB, JC        R0 = 25
    09  Factorial              Nested loops   R0 = 120
    10  Power Function         Exponentiation R0 = 32

Level 3: Memory & Arrays
    11  Array Sum              LOAD, R6:R7    R0 = 150
    12  Array Copy             LOAD, STORE    Array copied
    13  Find Element           Linear search  R0 = 2 (index)

Level 4: MMIO & Hardware (Advanced)
    14  Read Input             MMIO, AND      R0 = key state
    15  Read Mouse             MMIO           R0 = X, R1 = Y
    16  Draw Pixel             Framebuffer    Pixel at (10,10)
    17  Draw Line              Loop + FB      10 pixel line
    18  Animated Pixel         Input + FB     Moving pixel

More levels: Bit manipulation, subroutines, algorithms


COMMON PATTERNS
---------------

Loop N times:

    LOADI R0, 5
    loop:
        DEC R0
        JNZ loop

Access memory:

    LOADI R6, 0x20    ; addr high
    LOADI R7, 0x00    ; addr low
    LOAD  R0          ; read
    STORE R0          ; write

Multiply by addition:

    LOADI R2, 0       ; result
    loop:
        ADD R2, R0
        DEC R1
        JNZ loop


COMMON MMIO ADDRESSES
---------------------

Input:
    0xBF00  KEYS_STATE    bit 7=Down, 6=Up, 5=Left, 4=Right
    0xBF01  KEYS_PRESSED  Edge detection (cleared on read)
    0xBF02  MOUSE_X       0-127
    0xBF03  MOUSE_Y       0-127
    0xBF04  MOUSE_BUTTONS bit 0=left, 1=right, 2=middle

Timers:
    0xBF20  TICK_LOW      CPU tick counter low byte
    0xBF21  TICK_HIGH     CPU tick counter high byte
    0xBF22  FRAME_COUNT   Frame counter (60Hz)
    0xBF23  VSYNC         Write 1 to signal frame complete

Framebuffer:
    0xC000  Start of 128×128 display
    Address = 0xC000 + (y << 7) + x
    RGB332: RRRGGGBB (0xFF = white, 0x00 = black)


TIPS
----

- Always end with HALT
- Use -m 100 -d for quick testing
- Use -df to dump framebuffer
- R6:R7 form 16-bit address: (R6 << 8) | R7
- Many ops set Z (zero) and C (carry) flags
- Code at 0x0010+, data at 0x2000+


RESOURCES
---------

ISA:       ../specs/isa.txt
Assembler: ../specs/assembler.txt
Examples:  ../examples/
Solutions: SOLUTIONS.md
