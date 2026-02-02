; vim: set ft=asm:
; =============================================================================
; TINY16 TIMING UTILITIES
; =============================================================================
; VSYNC and frame timing macros
; =============================================================================

; -----------------------------------------------------------------------------
; READ_FRAME_COUNT dest - Read frame counter (increments at 60 Hz)
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_FRAME_COUNT dest
    LOADI R6, FRAME_COUNT_HI
    LOADI R7, FRAME_COUNT_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; WAIT_VSYNC last_frame_addr - Wait for next frame
; Clobbers: R0, R1, R6, R7
; -----------------------------------------------------------------------------
.macro WAIT_VSYNC last_frame_addr
@loop:
    LOADI R6, FRAME_COUNT_HI
    LOADI R7, FRAME_COUNT_LO
    LOAD R0, [R6:R7]
    LOADI R6, last_frame_addr >> 8
    LOADI R7, last_frame_addr & 0xFF
    LOAD R1, [R6:R7]
    CMP R0, R1
    JZ @loop
    STORE R0, [R6:R7]
.endmacro
