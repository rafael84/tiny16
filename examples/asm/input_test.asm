; Input Test - Move a Square with Keyboard
;
; Controls:
; - Arrow keys or WASD: Move sprite
; - Z or A button: Change color
;
; This demo shows:
; - Keyboard input handling (KEYS_STATE and KEYS_PRESSED)
; - PPU sprite rendering
; - Palette manipulation
; - Boundary checking
; - Using macros for cleaner input code

.include "tiny16.inc"

; =============================================================================
; Constants - Memory Map
; =============================================================================

; Data section base address
USER_DATA_BASE    = 0x4000
DATA_HI           = USER_DATA_BASE >> 8

; Data layout offsets
INITIALIZED_ADDR  = USER_DATA_BASE + 0
POS_X_ADDR        = USER_DATA_BASE + 1
POS_Y_ADDR        = USER_DATA_BASE + 2
LAST_FRAME_ADDR   = USER_DATA_BASE + 3


; Graphics memory
TILES_BASE        = GFX_TILES_BASE
OAM_BASE          = GFX_OAM_BASE
PALETTE_BASE      = GFX_PALETTE_BASE

; Derived addresses
PALETTE_COLOR1_ADDR = PALETTE_BASE + 1  ; Changed from +2 (now 1 byte per color)

; =============================================================================
; Constants - Input & Graphics
; =============================================================================

; Keyboard bit masks now provided by constants.inc (KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_A)
KEY_BUTTON_A      = KEY_A

; Game parameters
BOUNDARY          = 120
CENTER_POS        = BOUNDARY / 2
INITIALIZED_FLAG  = 0xAA
COLOR_TOGGLE_MASK = 0x1C
TILE_SIZE_BYTES   = 32
OAM_SPRITE_COUNT  = GFX_OAM_SPRITE_COUNT
SOLID_PIXEL_PAIR  = 0x11
TILE_SQUARE       = 0x00
SPRITE_ATTR_NONE  = 0x00

; Palette colors (RGB332 format)
BG_COLOR          = 0x03
DEFAULT_COLOR     = 0xFC

section .code

START:
    ; Check if already initialized
    LOAD16 R0, INITIALIZED_ADDR
    LOADI R1, INITIALIZED_FLAG
    CMP R0, R1
    JZ MAIN_LOOP

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
    SETADDR PALETTE_BASE
    LOADI R0, BG_COLOR
    STORE R0, [R6:R7]+        ; Color 0 (background)
    LOADI R0, DEFAULT_COLOR
    STORE R0, [R6:R7]         ; Color 1 (sprite)
    RET

; =============================================================================
; INIT_TILE - Set up solid square tile at index 0
; =============================================================================
INIT_TILE:
    MEMSET TILES_BASE, TILE_SIZE_BYTES, SOLID_PIXEL_PAIR
    RET

; =============================================================================
; INIT_OAM - Hide all 64 sprites
; =============================================================================
INIT_OAM:
    SETADDR OAM_BASE
    LOADI R1, OAM_SPRITE_COUNT

INIT_OAM_LOOP:
    OAM_HIDE_SPRITE
    DEC R1
    JNZ INIT_OAM_LOOP
    RET

; =============================================================================
; INIT_SPRITE - Initialize sprite position and mark initialized
; =============================================================================
INIT_SPRITE:
    SETADDR POS_X_ADDR
    LOADI R0, CENTER_POS
    STORE R0, [R6:R7]+
    LOADI R0, CENTER_POS
    STORE R0, [R6:R7]

    STORE16I INITIALIZED_FLAG, INITIALIZED_ADDR
    RET

; =============================================================================
; READ_INPUT - Read keyboard and update position/color
; =============================================================================
READ_INPUT:
    ; Load current position
    SETADDR POS_X_ADDR
    LOAD R1, [R6:R7]+
    LOAD R2, [R6:R7]

    READ_KEYS R0

    ; Check Up
    SKIP_IF_CLEAR R0, KEY_UP, CHECK_DOWN
    CMPI R2, 0
    JZ CHECK_DOWN
    DEC R2

CHECK_DOWN:
    SKIP_IF_CLEAR R0, KEY_DOWN, CHECK_LEFT
    CMPI R2, BOUNDARY
    JZ CHECK_LEFT
    INC R2

CHECK_LEFT:
    SKIP_IF_CLEAR R0, KEY_LEFT, CHECK_RIGHT
    CMPI R1, 0
    JZ CHECK_RIGHT
    DEC R1

CHECK_RIGHT:
    SKIP_IF_CLEAR R0, KEY_RIGHT, CHECK_BUTTON_A
    CMPI R1, BOUNDARY
    JZ CHECK_BUTTON_A
    INC R1

CHECK_BUTTON_A:
    READ_KEYS_PRESSED R3
    SKIP_IF_CLEAR R3, KEY_BUTTON_A, SAVE_POS
    ; Toggle palette color 1 (change square color)
    LOADI R6, PALETTE_COLOR1_ADDR >> 8
    LOADI R7, PALETTE_COLOR1_ADDR & 0xFF
    LOAD  R3, [R6:R7]
    LOADI R4, COLOR_TOGGLE_MASK
    XOR   R3, R4
    STORE R3, [R6:R7]

SAVE_POS:
    SETADDR POS_X_ADDR
    STORE R1, [R6:R7]+
    STORE R2, [R6:R7]
    RET

; =============================================================================
; UPDATE_OAM - Write current position to OAM entry 0
; =============================================================================
UPDATE_OAM:
    ; Load position
    SETADDR POS_X_ADDR
    LOAD R0, [R6:R7]+
    LOAD R1, [R6:R7]

    ; Write to OAM entry 0
    SETADDR OAM_BASE
    OAM_WRITE_SPRITE R1, R0, TILE_SQUARE, SPRITE_ATTR_NONE
    RET

; =============================================================================
; RENDER_FRAME - Trigger PPU render
; =============================================================================
RENDER_FRAME:
    PPU_SPRITES_ON
    RET

; =============================================================================
; WAIT_FRAME - Wait for next display frame
; =============================================================================
WAIT_FRAME:
    WAIT_VSYNC LAST_FRAME_ADDR
    RET

section .data

initialized:  DB 0     ; 0x4000
pos_x:        DB 0     ; 0x4001
pos_y:        DB 0     ; 0x4002
last_frame:   DB 0     ; 0x4003
