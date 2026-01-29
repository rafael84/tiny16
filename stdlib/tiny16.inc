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
; MODULES:
;   - constants.inc : Hardware constants, MMIO addresses, key definitions
;   - apu.inc       : Audio Processing Unit macros and note table
;
; =============================================================================

; Hardware constants and MMIO addresses
.include "constants.inc"

; Audio Processing Unit
.include "apu.inc"

; =============================================================================
; MEMORY & REGISTER UTILITIES
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

; =============================================================================
; STACK OPERATIONS
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

; =============================================================================
; ARITHMETIC & LOGIC UTILITIES
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

; =============================================================================
; INPUT UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; READ_KEYS dest - Read keyboard state
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_KEYS dest
    LOADI R6, KEYS_STATE_HI
    LOADI R7, KEYS_STATE_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_KEYS_PRESSED dest - Read edge-triggered key presses
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_KEYS_PRESSED dest
    LOADI R6, KEYS_PRESSED_HI
    LOADI R7, KEYS_PRESSED_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_MOUSE_X/Y dest - Read mouse coordinates
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_MOUSE_X dest
    LOADI R6, MOUSE_X_HI
    LOADI R7, MOUSE_X_LO
    LOAD dest, [R6:R7]
.endmacro

.macro READ_MOUSE_Y dest
    LOADI R6, MOUSE_Y_HI
    LOADI R7, MOUSE_Y_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_MOUSE dest_x, dest_y - Read both mouse coordinates
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_MOUSE dest_x, dest_y
    LOADI R6, MOUSE_X_HI
    LOADI R7, MOUSE_X_LO
    LOAD dest_x, [R6:R7]
    LOADI R6, MOUSE_Y_HI
    LOADI R7, MOUSE_Y_LO
    LOAD dest_y, [R6:R7]
.endmacro

; =============================================================================
; VSYNC & TIMING UTILITIES
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

; =============================================================================
; PPU & GRAPHICS UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; PPU_RENDER flags - Set PPU control register with custom flags
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro PPU_RENDER flags
    LOADI R0, flags
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; PPU_SPRITES_ON - Enable sprite rendering (most common operation)
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro PPU_SPRITES_ON
    LOADI R0, PPU_CTRL_SPRITES_ON
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; PPU_OFF - Disable all rendering
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro PPU_OFF
    XOR R0, R0
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    STORE R0, [R6:R7]
.endmacro

; =============================================================================
; OAM/SPRITE UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; OAM_WRITE_SPRITE y_reg, x_reg, tile, attr - Write sprite to OAM
; Requires: R6:R7 already set to OAM entry address
; Clobbers: R0, advances R6:R7 by 4 bytes
; -----------------------------------------------------------------------------
.macro OAM_WRITE_SPRITE y_reg, x_reg, tile, attr
    STORE y_reg, [R6:R7]+
    STORE x_reg, [R6:R7]+
    LOADI R0, tile
    STORE R0, [R6:R7]+
    LOADI R0, attr
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; OAM_HIDE_SPRITE - Hide sprite by setting Y to 0xFF
; Requires: R6:R7 set to OAM entry address
; Clobbers: R0, advances R7 by 4
; -----------------------------------------------------------------------------
.macro OAM_HIDE_SPRITE
    LOADI R0, 0xFF
    STORE R0, [R6:R7]
    INC R7
    INC R7
    INC R7
    INC R7
.endmacro

; -----------------------------------------------------------------------------
; OAM_SET_SPRITE sprite_idx - Set R6:R7 to OAM entry for sprite index
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro OAM_SET_SPRITE sprite_idx
    LOADI R6, GFX_OAM_BASE >> 8
    LOADI R7, (GFX_OAM_BASE & 0xFF) + (sprite_idx * 4)
.endmacro

; =============================================================================
; MEMORY OPERATIONS
; =============================================================================

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

; =============================================================================
; FRAMEBUFFER UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; FB_ADDR_FROM_XY - Compute framebuffer pointer from (x,y) in R0:R1
; Input: R0 = x (0-127), R1 = y (0-127)
; Output: R6:R7 = framebuffer address
; Clobbers: R3, R6, R7
; -----------------------------------------------------------------------------
.macro FB_ADDR_FROM_XY
    ; Low byte: (y<<7) + x
    MOV R7, R1
    TIMES 7 SHL R7
    ADD R7, R0
    ; High byte: 0xC0 + (y>>1)
    MOV R6, R1
    SHR R6
    LOADI R3, FRAMEBUFFER_HI
    ADD R6, R3
.endmacro

; -----------------------------------------------------------------------------
; DRAW_PIXEL x_reg, y_reg, color - Draw single pixel
; Clobbers: R0, R1, R3, R6, R7
; -----------------------------------------------------------------------------
.macro DRAW_PIXEL x_reg, y_reg, color
    MOV R0, x_reg
    MOV R1, y_reg
    ; Calculate framebuffer address
    MOV R7, R1
    TIMES 7 SHL R7
    ADD R7, R0
    MOV R6, R1
    SHR R6
    LOADI R3, FRAMEBUFFER_HI
    ADD R6, R3
    ; Store pixel
    LOADI R0, color
    STORE R0, [R6:R7]
.endmacro

; =============================================================================
; DEBUGGING & DEVELOPMENT UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; NOP - No operation
; -----------------------------------------------------------------------------
.macro NOP
    MOV R0, R0
.endmacro

; -----------------------------------------------------------------------------
; BREAKPOINT - Halt execution (debugging aid)
; -----------------------------------------------------------------------------
.macro BREAKPOINT
    HALT
.endmacro

; =============================================================================
; END OF TINY16 STANDARD LIBRARY
; =============================================================================
