; Standard Library for tiny16
;
; Register Usage Convention:
; - Macros preserve all registers unless documented otherwise
; - Use stack to save/restore temporary registers
; - Return values in R0 by convention

; =============================================================================
; Default MMIO Constants
; =============================================================================

MMIO_BASE           = 0xBF00
MMIO_HI             = MMIO_BASE >> 8

KEYS_STATE_ADDR     = MMIO_BASE + 0x00
KEYS_PRESSED_ADDR   = MMIO_BASE + 0x01
MOUSE_X_ADDR        = MMIO_BASE + 0x02
MOUSE_Y_ADDR        = MMIO_BASE + 0x03
MOUSE_BUTTONS_ADDR  = MMIO_BASE + 0x04
FRAME_COUNT_ADDR    = MMIO_BASE + 0x22
PPU_CTRL_ADDR       = MMIO_BASE + 0x30

KEYS_STATE_HI       = KEYS_STATE_ADDR >> 8
KEYS_STATE_LO       = KEYS_STATE_ADDR & 0xFF
KEYS_PRESSED_HI     = KEYS_PRESSED_ADDR >> 8
KEYS_PRESSED_LO     = KEYS_PRESSED_ADDR & 0xFF
MOUSE_X_HI          = MOUSE_X_ADDR >> 8
MOUSE_X_LO          = MOUSE_X_ADDR & 0xFF
MOUSE_Y_HI          = MOUSE_Y_ADDR >> 8
MOUSE_Y_LO          = MOUSE_Y_ADDR & 0xFF
MOUSE_BUTTONS_HI    = MOUSE_BUTTONS_ADDR >> 8
MOUSE_BUTTONS_LO    = MOUSE_BUTTONS_ADDR & 0xFF
FRAME_COUNT_HI      = FRAME_COUNT_ADDR >> 8
FRAME_COUNT_LO      = FRAME_COUNT_ADDR & 0xFF
PPU_CTRL_HI         = PPU_CTRL_ADDR >> 8
PPU_CTRL_LO         = PPU_CTRL_ADDR & 0xFF

; =============================================================================
; Default Graphics Memory Map
; =============================================================================

GFX_TILES_BASE      = 0x5000
GFX_OAM_BASE        = 0x7800
GFX_PALETTE_BASE    = 0x7900

GFX_OAM_SPRITE_COUNT = 64
GFX_OAM_ENTRY_SIZE   = 4

; PPU control bits
PPU_CTRL_SPRITES    = 0x02
PPU_CTRL_RENDER_NOW = 0x80
PPU_CTRL_SPRITES_ON = PPU_CTRL_SPRITES | PPU_CTRL_RENDER_NOW   ; 0x82

; =============================================================================
; Memory & Register Utilities
; =============================================================================

; Set R6:R7 to 16-bit address (clobbers R6, R7)
.macro SETADDR hi, lo
    LOADI R6, hi
    LOADI R7, lo
.endmacro

; Load from 16-bit address (clobbers R6, R7)
.macro LOAD16 dest, hi, lo
    LOADI R6, hi
    LOADI R7, lo
    LOAD dest, [R6:R7]
.endmacro

; Store to 16-bit address (clobbers R6, R7)
.macro STORE16 src, hi, lo
    LOADI R6, hi
    LOADI R7, lo
    STORE src, [R6:R7]
.endmacro

; Store immediate to 16-bit address (clobbers R0, R6, R7)
.macro STORE16I imm, hi, lo
    LOADI R0, imm
    LOADI R6, hi
    LOADI R7, lo
    STORE R0, [R6:R7]
.endmacro

; Zero a register
.macro CLEAR reg
    XOR reg, reg
.endmacro

; Test bit and jump if clear (preserves all registers)
.macro SKIP_IF_CLEAR reg, bitmask, label
    PUSH R3
    PUSH R4
    MOV R3, reg
    LOADI R4, bitmask
    AND R3, R4
    POP R4
    POP R3
    JZ label
.endmacro

; Push multiple registers
.macro PUSH2 r1, r2
    PUSH r1
    PUSH r2
.endmacro

.macro PUSH3 r1, r2, r3
    PUSH r1
    PUSH r2
    PUSH r3
.endmacro

.macro PUSH4 r1, r2, r3, r4
    PUSH r1
    PUSH r2
    PUSH r3
    PUSH r4
.endmacro

; Pop multiple registers (reverse order)
.macro POP2 r2, r1
    POP r1
    POP r2
.endmacro

.macro POP3 r3, r2, r1
    POP r1
    POP r2
    POP r3
.endmacro

.macro POP4 r4, r3, r2, r1
    POP r1
    POP r2
    POP r3
    POP r4
.endmacro

; Clamp value to range [0, max]
.macro CLAMP_MAX reg, max_val
    LOADI R5, max_val
    CMP reg, R5
    JC @skip
    MOV reg, R5
@skip:
.endmacro

; Clamp value to minimum
.macro CLAMP_MIN reg, min_val
    LOADI R5, min_val
    CMP reg, R5
    JNC @skip
    MOV reg, R5
@skip:
.endmacro

; Multiply by 4 (shift left twice)
.macro MUL4 reg
    ADD reg, reg
    ADD reg, reg
.endmacro

; Multiply by 8 (shift left three times)
.macro MUL8 reg
    ADD reg, reg
    ADD reg, reg
    ADD reg, reg
.endmacro

; =============================================================================
; Input Utilities
; =============================================================================

.macro READ_KEYS dest
    LOADI R6, KEYS_STATE_HI
    LOADI R7, KEYS_STATE_LO
    LOAD dest, [R6:R7]
.endmacro

.macro READ_KEYS_PRESSED dest
    LOADI R6, KEYS_PRESSED_HI
    LOADI R7, KEYS_PRESSED_LO
    LOAD dest, [R6:R7]
.endmacro

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

.macro READ_MOUSE_BUTTONS dest
    LOADI R6, MOUSE_BUTTONS_HI
    LOADI R7, MOUSE_BUTTONS_LO
    LOAD dest, [R6:R7]
.endmacro

; =============================================================================
; VSync Utilities
; =============================================================================

.macro READ_FRAME_COUNT dest
    LOADI R6, FRAME_COUNT_HI
    LOADI R7, FRAME_COUNT_LO
    LOAD dest, [R6:R7]
.endmacro

.macro WAIT_VSYNC last_frame_hi, last_frame_lo
@loop:
    LOADI R6, FRAME_COUNT_HI
    LOADI R7, FRAME_COUNT_LO
    LOAD R0, [R6:R7]
    LOADI R6, last_frame_hi
    LOADI R7, last_frame_lo
    LOAD R1, [R6:R7]
    CMP R0, R1
    JZ @loop
    STORE R0, [R6:R7]
.endmacro

; =============================================================================
; PPU Utilities
; =============================================================================

.macro PPU_RENDER flags
    LOADI R0, flags
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    STORE R0, [R6:R7]
.endmacro

.macro PPU_SPRITES_ON
    LOADI R0, PPU_CTRL_SPRITES_ON
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    STORE R0, [R6:R7]
.endmacro

; =============================================================================
; OAM/Sprite Utilities
; =============================================================================

; Write sprite to OAM (requires R6:R7 set to OAM address, clobbers R0)
.macro OAM_WRITE_SPRITE y_reg, x_reg, tile, attr
    STORE y_reg, [R6:R7]+
    STORE x_reg, [R6:R7]+
    LOADI R0, tile
    STORE R0, [R6:R7]+
    LOADI R0, attr
    STORE R0, [R6:R7]
.endmacro

; Hide sprite (requires R6:R7 set to OAM address, clobbers R0)
.macro OAM_HIDE_SPRITE
    LOADI R0, 0xFF
    STORE R0, [R6:R7]
    INC R7
    INC R7
    INC R7
    INC R7
.endmacro
