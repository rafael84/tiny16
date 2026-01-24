; Input Test - Move a square with arrow keys using PPU sprite
; Arrow keys or WASD to move
; Z/J to change color
;
; This demo shows:
; - Reading keyboard input (MMIO 0xBF00, 0xBF01)
; - PPU sprite rendering
; - Palette manipulation for color changes
; - Using constants for MMIO addresses and parameters

; =============================================================================
; Constants - Memory Map
; =============================================================================

; Data section addresses
INITIALIZED_ADDR  = 0x4000
POS_X_ADDR        = 0x4001
POS_Y_ADDR        = 0x4002
LAST_FRAME_ADDR   = 0x4003

; MMIO addresses
KEYS_STATE_ADDR   = 0xBF00
KEYS_PRESSED_ADDR = 0xBF01
FRAME_COUNT_ADDR  = 0xBF22
PPU_CTRL_ADDR     = 0xBF30

; Graphics memory
TILES_BASE        = 0x5000
OAM_BASE          = 0x7800
PALETTE_BASE      = 0x7900

; Address high/low bytes
INITIALIZED_HI    = 0x40
INITIALIZED_LO    = 0x00
POS_X_HI          = 0x40
POS_X_LO          = 0x01
POS_Y_HI          = 0x40
POS_Y_LO          = 0x02
LAST_FRAME_HI     = 0x40
LAST_FRAME_LO     = 0x03
KEYS_STATE_HI     = 0xBF
KEYS_STATE_LO     = 0x00
KEYS_PRESSED_HI   = 0xBF
KEYS_PRESSED_LO   = 0x01
FRAME_COUNT_HI    = 0xBF
FRAME_COUNT_LO    = 0x22
PPU_CTRL_HI       = 0xBF
PPU_CTRL_LO       = 0x30
OAM_HI            = 0x78
OAM_LO            = 0x00
PALETTE_HI        = 0x79
PALETTE_LO        = 0x00
PALETTE_COLOR1_LO = 0x02
TILES_HI          = 0x50
TILES_LO          = 0x00

; =============================================================================
; Constants - Input & Graphics
; =============================================================================

; Keyboard bit masks
KEY_UP            = 0x40
KEY_DOWN          = 0x80
KEY_LEFT          = 0x20
KEY_RIGHT         = 0x10
KEY_BUTTON_A      = 0x04

; Game parameters
BOUNDARY          = 120
CENTER_POS        = 60
INITIALIZED_FLAG  = 0xAA
PPU_SPRITES_RENDER = 0x82
COLOR_TOGGLE_MASK = 0x1C
TILE_SIZE_BYTES   = 32
OAM_SPRITE_COUNT  = 64
SOLID_PIXEL_PAIR  = 0x11
SPRITE_HIDDEN     = 0xFF
DEFAULT_COLOR     = 0xFC
BG_COLOR          = 0x03

section .code

START:
    ; Check if already initialized
    LOADI R6, INITIALIZED_HI
    LOADI R7, INITIALIZED_LO
    LOAD  R0, [R6:R7]
    LOADI R1, INITIALIZED_FLAG
    CMP   R0, R1
    JZ    MAIN_LOOP

    CALL  INIT_PALETTE
    CALL  INIT_TILE
    CALL  INIT_OAM
    CALL  INIT_SPRITE

MAIN_LOOP:
    CALL  WAIT_FRAME
    CALL  READ_INPUT
    CALL  UPDATE_OAM
    CALL  RENDER_FRAME
    JMP   MAIN_LOOP

; =============================================================================
; INIT_PALETTE - Set up palette colors
; =============================================================================
INIT_PALETTE:
    LOADI R6, PALETTE_HI
    LOADI R7, PALETTE_LO

    ; Color 0: dark blue background
    LOADI R0, BG_COLOR
    STORE R0, [R6:R7]
    INC   R7
    LOADI R0, 0x00
    STORE R0, [R6:R7]
    INC   R7

    ; Color 1: yellow - can be changed by button
    LOADI R0, DEFAULT_COLOR
    STORE R0, [R6:R7]
    INC   R7
    LOADI R0, 0x00
    STORE R0, [R6:R7]
    RET

; =============================================================================
; INIT_TILE - Set up solid square tile at index 0
; =============================================================================
INIT_TILE:
    LOADI R6, TILES_HI
    LOADI R7, TILES_LO
    LOADI R0, SOLID_PIXEL_PAIR
    LOADI R1, TILE_SIZE_BYTES

INIT_TILE_LOOP:
    STORE R0, [R6:R7]
    INC   R7
    DEC   R1
    JNZ   INIT_TILE_LOOP
    RET

; =============================================================================
; INIT_OAM - Hide all 64 sprites
; =============================================================================
INIT_OAM:
    LOADI R6, OAM_HI
    LOADI R7, OAM_LO
    LOADI R0, SPRITE_HIDDEN
    LOADI R1, OAM_SPRITE_COUNT

INIT_OAM_LOOP:
    STORE R0, [R6:R7]
    INC   R7
    INC   R7
    INC   R7
    INC   R7
    DEC   R1
    JNZ   INIT_OAM_LOOP
    RET

; =============================================================================
; INIT_SPRITE - Initialize sprite position and mark initialized
; =============================================================================
INIT_SPRITE:
    LOADI R6, POS_X_HI
    LOADI R7, POS_X_LO
    LOADI R0, CENTER_POS
    STORE R0, [R6:R7]
    INC   R7
    LOADI R0, CENTER_POS
    STORE R0, [R6:R7]

    ; Mark as initialized
    LOADI R7, INITIALIZED_LO
    LOADI R0, INITIALIZED_FLAG
    STORE R0, [R6:R7]
    RET

; =============================================================================
; READ_INPUT - Read keyboard and update position/color
; =============================================================================
READ_INPUT:
    ; Load current position
    LOADI R6, POS_X_HI
    LOADI R7, POS_X_LO
    LOAD  R1, [R6:R7]          ; R1 = pos_x
    INC   R7
    LOAD  R2, [R6:R7]          ; R2 = pos_y

    ; Read KEYS_STATE
    LOADI R6, KEYS_STATE_HI
    LOADI R7, KEYS_STATE_LO
    LOAD  R0, [R6:R7]          ; R0 = keys

    ; Check Up
    MOV   R3, R0
    LOADI R4, KEY_UP
    AND   R3, R4
    JZ    CHECK_DOWN
    LOADI R4, 0
    CMP   R2, R4
    JZ    CHECK_DOWN
    DEC   R2

CHECK_DOWN:
    MOV   R3, R0
    LOADI R4, KEY_DOWN
    AND   R3, R4
    JZ    CHECK_LEFT
    LOADI R4, BOUNDARY
    CMP   R2, R4
    JZ    CHECK_LEFT
    INC   R2

CHECK_LEFT:
    MOV   R3, R0
    LOADI R4, KEY_LEFT
    AND   R3, R4
    JZ    CHECK_RIGHT
    LOADI R4, 0
    CMP   R1, R4
    JZ    CHECK_RIGHT
    DEC   R1

CHECK_RIGHT:
    MOV   R3, R0
    LOADI R4, KEY_RIGHT
    AND   R3, R4
    JZ    CHECK_BUTTON_A
    LOADI R4, BOUNDARY
    CMP   R1, R4
    JZ    CHECK_BUTTON_A
    INC   R1

CHECK_BUTTON_A:
    ; Check A button - use KEYS_PRESSED for single press
    LOADI R6, KEYS_PRESSED_HI
    LOADI R7, KEYS_PRESSED_LO
    LOAD  R3, [R6:R7]
    LOADI R4, KEY_BUTTON_A
    AND   R3, R4
    JZ    SAVE_POS
    ; Toggle palette color 1 (change square color)
    LOADI R6, PALETTE_HI
    LOADI R7, PALETTE_COLOR1_LO
    LOAD  R3, [R6:R7]
    LOADI R4, COLOR_TOGGLE_MASK
    XOR   R3, R4
    STORE R3, [R6:R7]

SAVE_POS:
    LOADI R6, POS_X_HI
    LOADI R7, POS_X_LO
    STORE R1, [R6:R7]          ; pos_x
    INC   R7
    STORE R2, [R6:R7]          ; pos_y
    RET

; =============================================================================
; UPDATE_OAM - Write current position to OAM entry 0
; =============================================================================
UPDATE_OAM:
    ; Load position
    LOADI R6, POS_X_HI
    LOADI R7, POS_X_LO
    LOAD  R0, [R6:R7]          ; x
    INC   R7
    LOAD  R1, [R6:R7]          ; y

    ; Write to OAM entry 0
    LOADI R6, OAM_HI
    LOADI R7, OAM_LO
    STORE R1, [R6:R7]          ; Y
    INC   R7
    STORE R0, [R6:R7]          ; X
    INC   R7
    LOADI R0, 0x00    ; Tile index 0
    STORE R0, [R6:R7]
    INC   R7
    STORE R0, [R6:R7]          ; Attributes
    RET

; =============================================================================
; RENDER_FRAME - Trigger PPU render
; =============================================================================
RENDER_FRAME:
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    LOADI R0, PPU_SPRITES_RENDER
    STORE R0, [R6:R7]
    RET

; =============================================================================
; WAIT_FRAME - Wait for next display frame
; =============================================================================
WAIT_FRAME:
    LOADI R6, FRAME_COUNT_HI
    LOADI R7, FRAME_COUNT_LO
    LOAD  R0, [R6:R7]

    LOADI R6, LAST_FRAME_HI
    LOADI R7, LAST_FRAME_LO
    LOAD  R1, [R6:R7]

    CMP   R0, R1
    JZ    WAIT_FRAME

    STORE R0, [R6:R7]
    RET

section .data

initialized:  DB 0     ; 0x4000
pos_x:        DB 0     ; 0x4001
pos_y:        DB 0     ; 0x4002
last_frame:   DB 0     ; 0x4003
