; vim: set ft=asm:
; =============================================================================
; TINY16 STANDARD LIBRARY
; =============================================================================
;
; A comprehensive collection of macros and constants for Tiny16 assembly.
;
; QUICK START:
;   .include "../stdlib/tiny16.inc"
;
;   .section code
;   main:
;       CLEAR_SCREEN 0x03        ; Dark blue background
;       APU_INIT 12              ; Enable audio
;       APU_CH0_NOTE NOTE_A4, 12, APU_DUTY_50  ; Play A4
;       HALT
;
; CONVENTIONS:
;   - R6:R7 = Standard address pointer pair
;   - R0 = Return value (where applicable)
;   - Most macros clobber R6, R7 (documented per macro)
;   - Register pairs: high byte first, low byte second (e.g., R0:R1 = 0x1234)
;
; MODULES (auto-included):
;   - constants.inc  : Hardware constants, MMIO addresses, key definitions
;   - apu.inc        : Audio Processing Unit macros and note table
;   - memory.inc     : Memory and register utilities
;   - stack.inc      : Stack operation macros
;   - arithmetic.inc : Arithmetic, logic, and 16-bit operations
;   - input.inc      : Keyboard and mouse input
;   - timing.inc     : VSYNC and frame timing
;   - ppu.inc        : PPU, sprites/OAM, and framebuffer utilities
;
; OPTIONAL MODULES (include separately in code section):
;   - text.inc       : Text rendering as sprites (draw_text_line function)
;
; =============================================================================

.include "constants.inc"
.include "apu.inc"
.include "memory.inc"
.include "stack.inc"
.include "arithmetic.inc"
.include "input.inc"
.include "timing.inc"
.include "ppu.inc"
