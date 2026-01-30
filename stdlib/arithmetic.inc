; vim: set ft=asm:
; =============================================================================
; TINY16 ARITHMETIC UTILITIES
; =============================================================================
; Arithmetic, logic, and 16-bit operation macros
; =============================================================================

; -----------------------------------------------------------------------------
; ADDI reg, imm - Add immediate value to register
; Clobbers: R7
; -----------------------------------------------------------------------------
.macro ADDI reg, imm
    PUSH R7
    LOADI R7, imm
    ADD reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; SUBI reg, imm - Subtract immediate value from register
; Clobbers: R7
; -----------------------------------------------------------------------------
.macro SUBI reg, imm
    PUSH R7
    LOADI R7, imm
    SUB reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; CMPI reg, imm - Compare register with immediate value
; Clobbers: R7
; -----------------------------------------------------------------------------
.macro CMPI reg, imm
    PUSH R7
    LOADI R7, imm
    CMP reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; ANDI/ORI/XORI reg, imm - Bitwise operations with immediate
; Clobbers: R7
; -----------------------------------------------------------------------------
.macro ANDI reg, imm
    PUSH R7
    LOADI R7, imm
    AND reg, R7
    POP R7
.endmacro

.macro ORI reg, imm
    PUSH R7
    LOADI R7, imm
    OR reg, R7
    POP R7
.endmacro

.macro XORI reg, imm
    PUSH R7
    LOADI R7, imm
    XOR reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; Fast Multiplication (powers of 2)
; -----------------------------------------------------------------------------
.macro MUL2 reg
    ADD reg, reg
.endmacro

.macro MUL4 reg
    ADD reg, reg
    ADD reg, reg
.endmacro

.macro MUL8 reg
    ADD reg, reg
    ADD reg, reg
    ADD reg, reg
.endmacro

; -----------------------------------------------------------------------------
; Fast Division (powers of 2)
; -----------------------------------------------------------------------------
.macro DIV2 reg
    SHR reg
.endmacro

.macro DIV4 reg
    SHR reg
    SHR reg
.endmacro

.macro DIV8 reg
    SHR reg
    SHR reg
    SHR reg
.endmacro

; -----------------------------------------------------------------------------
; CLAMP_MAX reg, max_val - Clamp register to maximum [0, max]
; Clobbers: R5
; -----------------------------------------------------------------------------
.macro CLAMP_MAX reg, max_val
    PUSH R5
    LOADI R5, max_val
    CMP reg, R5
    JC @skip
    MOV reg, R5
@skip:
    POP R5
.endmacro

; -----------------------------------------------------------------------------
; CLAMP_MIN reg, min_val - Clamp register to minimum [min, 255]
; Clobbers: R5
; -----------------------------------------------------------------------------
.macro CLAMP_MIN reg, min_val
    PUSH R5
    LOADI R5, min_val
    CMP reg, R5
    JNC @skip
    MOV reg, R5
@skip:
    POP R5
.endmacro

; =============================================================================
; 16-BIT OPERATIONS
; =============================================================================

; -----------------------------------------------------------------------------
; ADD16 dst_hi, dst_lo, src_hi, src_lo - 16-bit addition with carry
; -----------------------------------------------------------------------------
.macro ADD16 dst_hi, dst_lo, src_hi, src_lo
    ADD dst_lo, src_lo
    JNC @no_carry
    INC dst_hi
@no_carry:
    ADD dst_hi, src_hi
.endmacro

; -----------------------------------------------------------------------------
; SUB16 dst_hi, dst_lo, src_hi, src_lo - 16-bit subtraction with borrow
; -----------------------------------------------------------------------------
.macro SUB16 dst_hi, dst_lo, src_hi, src_lo
    SUB dst_lo, src_lo
    JNC @no_borrow
    DEC dst_hi
@no_borrow:
    SUB dst_hi, src_hi
.endmacro

; -----------------------------------------------------------------------------
; INC16 reg_hi, reg_lo - 16-bit increment
; -----------------------------------------------------------------------------
.macro INC16 reg_hi, reg_lo
    INC reg_lo
    JNZ @done
    INC reg_hi
@done:
.endmacro

; -----------------------------------------------------------------------------
; DEC16 reg_hi, reg_lo - 16-bit decrement
; -----------------------------------------------------------------------------
.macro DEC16 reg_hi, reg_lo
    DEC reg_lo
    LOADI R0, 0xFF
    CMP reg_lo, R0
    JNZ @done
    DEC reg_hi
@done:
.endmacro

; -----------------------------------------------------------------------------
; CMP16 cmp_hi, cmp_lo, val_hi, val_lo - 16-bit comparison
; -----------------------------------------------------------------------------
.macro CMP16 cmp_hi, cmp_lo, val_hi, val_lo
    CMP cmp_hi, val_hi
    JNZ @done
    CMP cmp_lo, val_lo
@done:
.endmacro

; -----------------------------------------------------------------------------
; LOAD16I reg_hi, reg_lo, value16 - Load 16-bit immediate into register pair
; -----------------------------------------------------------------------------
.macro LOAD16I reg_hi, reg_lo, value16
    LOADI reg_hi, value16 >> 8
    LOADI reg_lo, value16 & 0xFF
.endmacro
