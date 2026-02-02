; vim: set ft=asm:
; =============================================================================
; TINY16 MEMORY UTILITIES
; =============================================================================
; Memory and register manipulation macros
; =============================================================================

; -----------------------------------------------------------------------------
; SETADDR addr16 - Set R6:R7 to 16-bit address
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro SETADDR addr16
    LOADI R6, addr16 >> 8
    LOADI R7, addr16 & 0xFF
.endmacro

; -----------------------------------------------------------------------------
; LOAD16 dest, addr16 - Load from 16-bit address
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro LOAD16 dest, addr16
    LOADI R6, addr16 >> 8
    LOADI R7, addr16 & 0xFF
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; STORE16 src, addr16 - Store register to 16-bit address
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro STORE16 src, addr16
    LOADI R6, addr16 >> 8
    LOADI R7, addr16 & 0xFF
    STORE src, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; STORE16I imm, addr16 - Store immediate to 16-bit address
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro STORE16I imm, addr16
    LOADI R0, imm
    LOADI R6, addr16 >> 8
    LOADI R7, addr16 & 0xFF
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; CLEAR reg - Zero a register (faster than LOADI reg, 0)
; -----------------------------------------------------------------------------
.macro CLEAR reg
    XOR reg, reg
.endmacro

; -----------------------------------------------------------------------------
; SET reg, value - Set register to immediate
; -----------------------------------------------------------------------------
.macro SET reg, value
    LOADI reg, value
.endmacro

; -----------------------------------------------------------------------------
; MEMSET addr16, count, value - Fill memory with value
; Clobbers: R0, R1, R6, R7
; -----------------------------------------------------------------------------
.macro MEMSET addr16, count, value
    LOADI R6, addr16 >> 8
    LOADI R7, addr16 & 0xFF
    LOADI R0, value
    LOADI R1, count
@loop:
    STORE R0, [R6:R7]+
    DEC R1
    JNZ @loop
.endmacro

; -----------------------------------------------------------------------------
; MEMCPY dst_addr, src_addr, count - Copy memory block
; Clobbers: R0, R1, R2, R3, R6, R7
; -----------------------------------------------------------------------------
.macro MEMCPY dst_addr, src_addr, count
    PUSH R2
    PUSH R3
    LOADI R2, src_addr >> 8
    LOADI R3, src_addr & 0xFF
    LOADI R6, dst_addr >> 8
    LOADI R7, dst_addr & 0xFF
    LOADI R1, count
@loop:
    LOAD R0, [R2:R3]+
    STORE R0, [R6:R7]+
    DEC R1
    JNZ @loop
    POP R3
    POP R2
.endmacro

; -----------------------------------------------------------------------------
; CLEAR_SCREEN color - Fill entire framebuffer with color
; Clobbers: R0, R1, R6, R7
; -----------------------------------------------------------------------------
.macro CLEAR_SCREEN color
    LOADI R0, color
    LOADI R6, FRAMEBUFFER_HI
    LOADI R7, FRAMEBUFFER_LO
@loop:
    STORE R0, [R6:R7]
    INC R7
    JNZ @loop
    INC R6
    XOR R1, R1
    CMP R6, R1
    JNZ @loop
.endmacro
