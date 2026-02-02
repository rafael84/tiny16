; vim: set ft=asm:
; =============================================================================
; TINY16 STACK UTILITIES
; =============================================================================
; Stack operation macros for multiple registers
; =============================================================================

.macro PUSH2 r1, r2
    PUSH r1
    PUSH r2
.endmacro

.macro POP2 r2, r1
    POP r1
    POP r2
.endmacro

.macro PUSH3 r1, r2, r3
    PUSH r1
    PUSH r2
    PUSH r3
.endmacro

.macro POP3 r3, r2, r1
    POP r1
    POP r2
    POP r3
.endmacro

.macro PUSH4 r1, r2, r3, r4
    PUSH r1
    PUSH r2
    PUSH r3
    PUSH r4
.endmacro

.macro POP4 r4, r3, r2, r1
    POP r1
    POP r2
    POP r3
    POP r4
.endmacro
