; Input Test - Move a square with arrow keys using PPU sprite
; Arrow keys or WASD to move
; Z/J to change color
;
; This demo shows:
; - Reading keyboard input (MMIO 0xBF00, 0xBF01)
; - PPU sprite rendering
; - Palette manipulation for color changes

section .code

START:
    ; Check if already initialized
    LOADI R6, 0x40
    LOADI R7, 0x00
    LOAD  R0, [R6:R7]
    LOADI R1, 0xAA
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
    LOADI R6, 0x79    ; PALETTE_BASE high
    LOADI R7, 0x00    ; PALETTE_BASE low

    ; Color 0: dark blue background (0x03)
    LOADI R0, 0x03
    STORE R0, [R6:R7]
    INC   R7
    LOADI R0, 0x00
    STORE R0, [R6:R7]
    INC   R7

    ; Color 1: yellow (0xFC) - can be changed by button
    LOADI R0, 0xFC
    STORE R0, [R6:R7]
    INC   R7
    LOADI R0, 0x00
    STORE R0, [R6:R7]
    RET

; =============================================================================
; INIT_TILE - Set up solid square tile at index 0
; =============================================================================
INIT_TILE:
    LOADI R6, 0x50    ; TILES_BASE high
    LOADI R7, 0x00    ; TILES_BASE low
    LOADI R0, 0x11    ; Two pixels of color 1
    LOADI R1, 32      ; 32 bytes per tile

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
    LOADI R6, 0x78    ; OAM_BASE high
    LOADI R7, 0x00    ; OAM_BASE low
    LOADI R0, 0xFF    ; Y = 0xFF means hidden
    LOADI R1, 64

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
    LOADI R6, 0x40
    LOADI R7, 0x01
    LOADI R0, 60      ; pos_x = 60 (center)
    STORE R0, [R6:R7]
    INC   R7
    LOADI R0, 60      ; pos_y = 60 (center)
    STORE R0, [R6:R7]

    ; Mark as initialized
    LOADI R7, 0x00
    LOADI R0, 0xAA
    STORE R0, [R6:R7]
    RET

; =============================================================================
; READ_INPUT - Read keyboard and update position/color
; =============================================================================
READ_INPUT:
    ; Load current position
    LOADI R6, 0x40
    LOADI R7, 0x01
    LOAD  R1, [R6:R7]          ; R1 = pos_x
    INC   R7
    LOAD  R2, [R6:R7]          ; R2 = pos_y

    ; Read KEYS_STATE
    LOADI R6, 0xBF
    LOADI R7, 0x00
    LOAD  R0, [R6:R7]          ; R0 = keys

    ; Check Up (bit 6)
    MOV   R3, R0
    LOADI R4, 0x40
    AND   R3, R4
    JZ    CHECK_DOWN
    LOADI R4, 0
    CMP   R2, R4
    JZ    CHECK_DOWN
    DEC   R2

CHECK_DOWN:
    MOV   R3, R0
    LOADI R4, 0x80
    AND   R3, R4
    JZ    CHECK_LEFT
    LOADI R4, 120
    CMP   R2, R4
    JZ    CHECK_LEFT
    INC   R2

CHECK_LEFT:
    MOV   R3, R0
    LOADI R4, 0x20
    AND   R3, R4
    JZ    CHECK_RIGHT
    LOADI R4, 0
    CMP   R1, R4
    JZ    CHECK_RIGHT
    DEC   R1

CHECK_RIGHT:
    MOV   R3, R0
    LOADI R4, 0x10
    AND   R3, R4
    JZ    CHECK_BUTTON_A
    LOADI R4, 120
    CMP   R1, R4
    JZ    CHECK_BUTTON_A
    INC   R1

CHECK_BUTTON_A:
    ; Check A button (bit 2) - use KEYS_PRESSED for single press
    LOADI R6, 0xBF
    LOADI R7, 0x01
    LOAD  R3, [R6:R7]
    LOADI R4, 0x04
    AND   R3, R4
    JZ    SAVE_POS
    ; Toggle palette color 1 (change square color)
    LOADI R6, 0x79
    LOADI R7, 0x02    ; Color 1 at offset 2
    LOAD  R3, [R6:R7]
    LOADI R4, 0x1C
    XOR   R3, R4
    STORE R3, [R6:R7]

SAVE_POS:
    LOADI R6, 0x40
    LOADI R7, 0x01
    STORE R1, [R6:R7]          ; pos_x
    INC   R7
    STORE R2, [R6:R7]          ; pos_y
    RET

; =============================================================================
; UPDATE_OAM - Write current position to OAM entry 0
; =============================================================================
UPDATE_OAM:
    ; Load position
    LOADI R6, 0x40
    LOADI R7, 0x01
    LOAD  R0, [R6:R7]          ; x
    INC   R7
    LOAD  R1, [R6:R7]          ; y

    ; Write to OAM entry 0
    LOADI R6, 0x78
    LOADI R7, 0x00
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
    LOADI R6, 0xBF
    LOADI R7, 0x30
    LOADI R0, 0x82    ; ENABLE_SPRITES | RENDER_NOW
    STORE R0, [R6:R7]
    RET

; =============================================================================
; WAIT_FRAME - Wait for next display frame
; =============================================================================
WAIT_FRAME:
    LOADI R6, 0xBF
    LOADI R7, 0x22    ; FRAME_COUNT
    LOAD  R0, [R6:R7]

    LOADI R6, 0x40
    LOADI R7, 0x03    ; last_frame
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
