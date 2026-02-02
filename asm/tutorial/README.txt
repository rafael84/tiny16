Tiny16 Assembly Tutorial
========================

Progressive course covering assembly fundamentals and tiny16 features.

QUICK START:
  cd asm/tutorial/
  ../../build/tiny16-asm 01_load_and_halt.asm 01.tiny16
  ../../build/tiny16-emu 01.tiny16 -m 100 -d
  # or from repo root:
  #   ./eval.sh asm/tutorial/01_load_and_halt.asm

EMULATOR FLAGS:
  -m <n>  max instructions   -d  dump state
  -t      trace all          -tm trace memory


LEARNING PATH
=============

Level 1: Fundamentals (01-08) - Registers, loading, arithmetic
    01  Load and Halt         LOADI, HALT          R0 = 42
    02  Add Two Numbers        ADD                  R0 = 15
    03  Copy Register          MOV                  R1 = R0
    04  Simple Subtraction     SUB                  R0 = 5
    05  Increment/Decrement    INC, DEC             R0 = 11
    06  Zero a Register        XOR                  R0 = 0
    07  Double a Number        SHL or ADD           R0 = 20
    08  Halve a Number         SHR                  R0 = 5


Level 2: Control Flow (09-16) - Branching, loops, conditionals
    09  Unconditional Jump     JMP                  Infinite loop
    10  Simple Loop            DEC, JNZ             Count down
    11  Count Up               INC, CMP, JZ         Count to 10
    12  Compare Numbers        CMP, JZ, JC          R0 = max(5,8)
    13  If-Else Logic          CMP, JC, JMP         Branching
    14  For Loop               Loop N times         Sum 1..N
    15  While Loop             Condition at start   Find first >100
    16  Nested Loops           Loop in loop         2D counter


Level 3: Memory & Data (17-24) - Register pairs, arrays, addressing
    17  Load from Memory       LOAD [PAIR]          Read byte
    18  Store to Memory        STORE [PAIR]         Write byte
    19  Array Access           LOAD with offset     Read array[2]
    20  Post-Increment         LOAD [PAIR]+         Array traversal
    21  Post-Decrement         LOAD [PAIR]-         Reverse traverse
    22  Offset Addressing      LOAD [PAIR +/- N]    Struct/stack access
    23  Array Sum              Loop + LOAD          Sum array
    24  Array Copy             LOAD + STORE         Copy 10 bytes


Level 4: Stack & Subroutines (25-32) - Functions, calls, recursion
    25  Push and Pop           PUSH, POP            Stack basics
    26  Preserve Registers     PUSH/POP in loop     Save/restore
    27  Simple Subroutine      CALL, RET            Function call
    28  Subroutine with Args   Pass via registers   Add(a, b)
    29  Nested Calls           CALL in CALL         Call chain
    30  Recursive Countdown    Recursion            Countdown(n)
    31  Recursive Factorial    Recursion + stack    Factorial(n)
    32  Stack Frame            SP manipulation      Local variables


Level 5: Advanced Arithmetic (33-38) - Multi-byte, multiply, divide
    33  16-bit Addition        ADC                  Add two words
    34  16-bit Subtraction     SBC                  Subtract words
    35  8-bit Multiply         Repeated addition    a × b
    36  Fast Multiply          Shift and add        a × b
    37  8-bit Divide           Repeated subtract    a ÷ b
    38  Modulo Operation       Division remainder   a % b


Level 6: Bit Manipulation (39-44) - Bitwise ops, masking, shifts
    39  Set/Clear Bits         OR, AND              Bit manipulation
    40  Test Bit               AND, JZ              Check if set
    41  Toggle Bits            XOR                  Flip bits
    42  Count Set Bits         Shift + test         Population count
    43  Rotate Left            Manual rotation      Circular shift
    44  Byte Swap              Nibble manipulation  Swap high/low


Level 7: Assembler (45-48) - Constants, macros, includes
    45  Constants/Expressions  Named values         Computed values
    46  Sections & ORG         Data placement       Custom layout
    47  Macros                 Reusable code        Parameterized
    48  Includes               Shared definitions   Multi-file


COMMON PATTERNS
===============
Loop (down):  LOADI R0,10 / loop: ... / DEC R0 / JNZ loop
Loop (up):    LOADI R0,0 / LOADI R1,10 / loop: ... / INC R0 /
              CMP R0,R1 / JNZ loop
Memory:       LOADI R6,hi / LOADI R7,lo / LOAD R0,[R6:R7] /
              STORE R1,[R6:R7]
Array:        LOADI R6:R7,addr / LOADI R0,cnt / loop:
              LOAD R1,[R6:R7]+ / ... / DEC R0 / JNZ loop
Function:     LOADI R0,arg / CALL func / HALT
              func: PUSH R1 / ... / POP R1 / RET
Compare:      CMP R0,R1 / JZ equal / JC less / JMP greater


REGISTERS & FLAGS
=================
R0,R1: return/temp   R2,R3: args/temp
R4,R5: counters      R6,R7: addressing (default pair)
Pairs: R0:R1, R2:R3, R4:R5, R6:R7

Z flag: Set when result=0
        Updated by: ADD SUB ADC SBC INC DEC AND OR XOR CMP SHL SHR
C flag: Set on overflow/borrow and by shifts
        Cleared by INC/DEC/logic ops. Used for multi-byte math.
CMP:    CMP R0,R1 sets Z if equal, C if R0<R1
        Then: JZ equal / JC less / JNC greater_or_equal


MEMORY MAP
==========
0x0000-0x000F: Signature         0x0010-0x3FFF: Code
0x4000-0x4FFF: Data              0x8000-0xBEFF: Stack (down)


ASSEMBLER FEATURES
==================
Sections:   section .code (0x0010)  section .data (0x4000)
Data:       msg: DB "Hello\0"   values: DB 1,2,3,0xFF
Repeat:     TIMES 4 SHR R0   buffer: TIMES 256 DB 0
Comments:   ; semicolon to end of line
Labels:     loop: INC R0 / JMP loop
Numbers:    42 (dec)  0x2A (hex)  0b00101010 (bin)
Constants:  VALUE = (6+2)*3   MASK = (1<<4)|0x03
            LOADI R0, VALUE
ORG:        section .data / ORG 0x4010 / value: DB 0x2A
Macros:     .macro ADDI reg,imm / LOADI R7,imm / ADD reg,R7 /
            .endmacro    Usage: ADDI R0,5
Includes:   .include "file.inc"


TIPS & DEBUGGING
================
- Always end with HALT. Use -m 100 -d for quick testing.
- Use -t to trace execution.
- XOR R,R zeros register. CMP preserves values (SUB doesn't).
- INC/DEC clear carry flag.
- Post-increment [R6:R7]+ simplifies array loops.
- Stack grows down from 0xBEFF.
- Common errors: missing HALT, wrong pair (default is R6:R7),
  infinite loops (use -m), stack overflow

RESOURCES:
  ../../specs/tiny16-asm.txt
  ../../specs/tiny16-vm.txt
  ../../examples/*.asm
  Solutions are at the bottom of each tutorial file

ADVANCED:
  JC/JNC for overflow/comparisons
  [PAIR]- for reverse traversal
  TIMES for buffers

Start with 01 and work your way up!
