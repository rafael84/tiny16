; vim: set ft=asm:
; =============================================================================
; TINY16 PPU & GRAPHICS UTILITIES
; =============================================================================
; Picture Processing Unit, sprite/OAM, and framebuffer macros
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
    times 4 INC R7
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
    times 7 SHL R7
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
    times 7 SHL R7
    ADD R7, R0
    MOV R6, R1
    SHR R6
    LOADI R3, FRAMEBUFFER_HI
    ADD R6, R3
    ; Store pixel
    LOADI R0, color
    STORE R0, [R6:R7]
.endmacro
