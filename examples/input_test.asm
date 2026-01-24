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

.include "macros.inc"

; =============================================================================
; Constants - Memory Map
; =============================================================================

; Data section base address
USER_DATA_BASE    = 0x4000

; Data layout offsets
INITIALIZED_ADDR  = USER_DATA_BASE + 0
POS_X_ADDR        = USER_DATA_BASE + 1
POS_Y_ADDR        = USER_DATA_BASE + 2
LAST_FRAME_ADDR   = USER_DATA_BASE + 3


; Graphics memory
TILES_BASE        = 0x5000
OAM_BASE          = 0x7800
PALETTE_BASE      = 0x7900

; Address high/low bytes (computed from full addresses)
INITIALIZED_HI    = INITIALIZED_ADDR >> 8
INITIALIZED_LO    = INITIALIZED_ADDR & 0xFF
POS_X_HI          = POS_X_ADDR >> 8
POS_X_LO          = POS_X_ADDR & 0xFF
POS_Y_HI          = POS_Y_ADDR >> 8
POS_Y_LO          = POS_Y_ADDR & 0xFF
LAST_FRAME_HI     = LAST_FRAME_ADDR >> 8
LAST_FRAME_LO     = LAST_FRAME_ADDR & 0xFF
OAM_HI            = OAM_BASE >> 8
OAM_LO            = OAM_BASE & 0xFF
PALETTE_HI        = PALETTE_BASE >> 8
PALETTE_LO        = PALETTE_BASE & 0xFF
PALETTE_COLOR1_LO = PALETTE_LO + 2
TILES_HI          = TILES_BASE >> 8
TILES_LO          = TILES_BASE & 0xFF

; =============================================================================
; Constants - Input & Graphics
; =============================================================================

; Keyboard bit masks (bit positions in KEYS_STATE/KEYS_PRESSED)
KEY_UP            = 1 << 6
KEY_DOWN          = 1 << 7
KEY_LEFT          = 1 << 5
KEY_RIGHT         = 1 << 4
KEY_BUTTON_A      = 1 << 2

; Game parameters
BOUNDARY          = 120
CENTER_POS        = BOUNDARY / 2
INITIALIZED_FLAG  = 0xAA
COLOR_TOGGLE_MASK = 0x1C
TILE_SIZE_BYTES   = 32
OAM_SPRITE_COUNT  = 64
SOLID_PIXEL_PAIR  = 0x11
SPRITE_HIDDEN     = 0xFF

; PPU control: sprites enabled (0x02) | render now (0x80)
PPU_SPRITES_RENDER = 0x02 | 0x80

; Palette colors (RGB332 format)
BG_COLOR          = 0x03
DEFAULT_COLOR     = 0xFC

section .code

START:
    ; Check if already initialized
    LOAD16 R0, INITIALIZED_HI, INITIALIZED_LO
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
    SETADDR PALETTE_HI, PALETTE_LO
    LOADI R0, BG_COLOR
    STORE R0, [R6:R7]+
    CLEAR R0
    STORE R0, [R6:R7]+
    LOADI R0, DEFAULT_COLOR
    STORE R0, [R6:R7]+
    CLEAR R0
    STORE R0, [R6:R7]
    RET

; =============================================================================
; INIT_TILE - Set up solid square tile at index 0
; =============================================================================
INIT_TILE:
    SETADDR TILES_HI, TILES_LO
    LOADI R0, SOLID_PIXEL_PAIR
    LOADI R1, TILE_SIZE_BYTES

INIT_TILE_LOOP:
    STORE R0, [R6:R7]+
    DEC R1
    JNZ INIT_TILE_LOOP
    RET

; =============================================================================
; INIT_OAM - Hide all 64 sprites
; =============================================================================
INIT_OAM:
    SETADDR OAM_HI, OAM_LO
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
    SETADDR POS_X_HI, POS_X_LO
    LOADI R0, CENTER_POS
    STORE R0, [R6:R7]+
    LOADI R0, CENTER_POS
    STORE R0, [R6:R7]

    STORE16I INITIALIZED_FLAG, INITIALIZED_HI, INITIALIZED_LO
    RET

; =============================================================================
; READ_INPUT - Read keyboard and update position/color
; =============================================================================
READ_INPUT:
    ; Load current position
    SETADDR POS_X_HI, POS_X_LO
    LOAD R1, [R6:R7]+
    LOAD R2, [R6:R7]

    READ_KEYS R0

    ; Check Up
    SKIP_IF_CLEAR R0, KEY_UP, CHECK_DOWN
    LOADI R5, 0
    CMP R2, R5
    JZ CHECK_DOWN
    DEC R2

CHECK_DOWN:
    SKIP_IF_CLEAR R0, KEY_DOWN, CHECK_LEFT
    LOADI R5, BOUNDARY
    CMP R2, R5
    JZ CHECK_LEFT
    INC R2

CHECK_LEFT:
    SKIP_IF_CLEAR R0, KEY_LEFT, CHECK_RIGHT
    LOADI R5, 0
    CMP R1, R5
    JZ CHECK_RIGHT
    DEC R1

CHECK_RIGHT:
    SKIP_IF_CLEAR R0, KEY_RIGHT, CHECK_BUTTON_A
    LOADI R5, BOUNDARY
    CMP R1, R5
    JZ CHECK_BUTTON_A
    INC R1

CHECK_BUTTON_A:
    READ_KEYS_PRESSED R3
    SKIP_IF_CLEAR R3, KEY_BUTTON_A, SAVE_POS
    ; Toggle palette color 1 (change square color)
    LOAD16 R3, PALETTE_HI, PALETTE_COLOR1_LO
    LOADI R4, COLOR_TOGGLE_MASK
    XOR R3, R4
    STORE16 R3, PALETTE_HI, PALETTE_COLOR1_LO

SAVE_POS:
    SETADDR POS_X_HI, POS_X_LO
    STORE R1, [R6:R7]+
    STORE R2, [R6:R7]
    RET

; =============================================================================
; UPDATE_OAM - Write current position to OAM entry 0
; =============================================================================
UPDATE_OAM:
    ; Load position
    SETADDR POS_X_HI, POS_X_LO
    LOAD R0, [R6:R7]+
    LOAD R1, [R6:R7]

    ; Write to OAM entry 0
    SETADDR OAM_HI, OAM_LO
    OAM_WRITE_SPRITE R1, R0, 0x00, 0x00
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
    WAIT_VSYNC LAST_FRAME_HI, LAST_FRAME_LO
    RET

section .data

initialized:  DB 0     ; 0x4000
pos_x:        DB 0     ; 0x4001
pos_y:        DB 0     ; 0x4002
last_frame:   DB 0     ; 0x4003
