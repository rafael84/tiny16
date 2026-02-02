; Scroll Demo - PPU Tilemap Scrolling
;
; This demo demonstrates:
; - PPU scroll_x and scroll_y registers for smooth tilemap scrolling
; - Auto-scrolling with arrow key override
; - Wrapping at tilemap boundaries (256x256 pixels)
;
; Controls:
;   Arrow keys: Override auto-scroll direction
;
; The visible screen is 128x128 pixels, but the tilemap is 256x256 pixels.
;

.include "tiny16.inc"

; =============================================================================
; Constants - Memory Map
; =============================================================================

USER_DATA_BASE     = 0x4000
TILE_DATA_ADDR     = 0x5000
TILEMAP_ADDR       = 0x7000
PALETTE_ADDR       = 0x9200

; Data layout offsets
LAST_FRAME_ADDR    = USER_DATA_BASE + 0
SCROLL_X_VAR       = USER_DATA_BASE + 1
SCROLL_Y_VAR       = USER_DATA_BASE + 2

; =============================================================================
; Constants - Game Parameters
; =============================================================================

SCROLL_SPEED       = 1              ; Pixels per frame
PPU_CTRL_BG_ON     = 0x81           ; Enable BG (0x01) + Render (0x80)

; Key bits now provided by constants.inc

section .code

START:
    ; Initialize scroll position (start at 64 to verify scrolling works)
    STORE16I 64, SCROLL_X_VAR
    STORE16I 64, SCROLL_Y_VAR

    CALL  INIT_TILEMAP  ; Initialize tilemap with a pattern

MAIN_LOOP:
    WAIT_VSYNC LAST_FRAME_ADDR ; Wait for next frame
    CALL  UPDATE_SCROLL ; Handle input or auto-scroll

    ; Apply scroll to PPU registers
    LOAD16 R0, SCROLL_X_VAR
    LOADI R6, PPU_SCROLL_X_HI
    LOADI R7, PPU_SCROLL_X_LO
    STORE R0, [R6:R7]

    LOAD16 R0, SCROLL_Y_VAR
    LOADI R6, PPU_SCROLL_Y_HI
    LOADI R7, PPU_SCROLL_Y_LO
    STORE R0, [R6:R7]

    ; Render frame
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    LOADI R0, PPU_CTRL_BG_ON
    STORE R0, [R6:R7]

    JMP   MAIN_LOOP

; =============================================================================
; INIT_TILEMAP - Create a 4-quadrant pattern for visible scrolling
; Top-left: tile 3, Top-right: tile 4, Bottom-left: tile 5, Bottom-right: tile 6
; Tilemap is 128 tiles wide, each row = 256 bytes (one page)
; Address = 0x7000 + row*256 + col*2
; =============================================================================
INIT_TILEMAP:
    LOADI R4, 0               ; Row counter

FILL_ROW_LOOP:
    ; Set address for start of this row: 0x70 + row, 0x00
    LOADI R6, 0x70
    ADD   R6, R4              ; R6 = 0x70 + row
    LOADI R7, 0x00            ; R7 = 0 (start of row)
    LOADI R2, 0               ; Column counter

FILL_COL_LOOP:
    ; Determine quadrant based on row and col (16x16 tile boundary)
    ; tile = 3 + (row >= 16 ? 2 : 0) + (col >= 16 ? 1 : 0)
    LOADI R0, 3               ; Base tile

    ; Check if row >= 16 (bottom half)
    LOADI R1, 16
    CMP   R4, R1
    JC    CHECK_COL           ; row < 16, skip
    ADDI  R0, 2               ; Add 2 for bottom half

CHECK_COL:
    ; Check if col >= 16 (right half)
    CMP   R2, R1
    JC    STORE_TILE          ; col < 16, skip
    INC   R0                  ; Add 1 for right half

STORE_TILE:
    ; Store tile index and attribute
    STORE R0, [R6:R7]+        ; tile index
    LOADI R1, 0x00            ; no flip
    STORE R1, [R6:R7]+        ; attribute

    ; Next column
    INC   R2
    LOADI R1, 32
    CMP   R2, R1
    JNZ   FILL_COL_LOOP

    ; Next row
    INC   R4
    LOADI R1, 32
    CMP   R4, R1
    JNZ   FILL_ROW_LOOP

    RET

; =============================================================================
; UPDATE_SCROLL - Auto-scroll diagonally, with arrow key overrides
; =============================================================================
UPDATE_SCROLL:
    ; Read keyboard
    LOADI R6, KEYS_STATE_HI
    LOADI R7, KEYS_STATE_LO
    LOAD  R3, [R6:R7]         ; R3 = key state

    ; Check if any arrow keys are pressed
    LOADI R0, KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT
    MOV   R1, R3
    AND   R1, R0
    JNZ   HANDLE_KEYS         ; If any key pressed, handle manually

    ; No keys: auto-scroll diagonally (down-right)
    LOAD16 R0, SCROLL_X_VAR
    ADDI  R0, SCROLL_SPEED
    STORE16 R0, SCROLL_X_VAR

    LOAD16 R0, SCROLL_Y_VAR
    ADDI  R0, SCROLL_SPEED
    STORE16 R0, SCROLL_Y_VAR
    RET

HANDLE_KEYS:
    ; Check UP
    MOV   R0, R3
    LOADI R1, KEY_UP
    AND   R0, R1
    JZ    CHECK_KEY_DOWN
    LOAD16 R0, SCROLL_Y_VAR
    SUBI  R0, SCROLL_SPEED
    STORE16 R0, SCROLL_Y_VAR

CHECK_KEY_DOWN:
    MOV   R0, R3
    LOADI R1, KEY_DOWN
    AND   R0, R1
    JZ    CHECK_KEY_LEFT
    LOAD16 R0, SCROLL_Y_VAR
    ADDI  R0, SCROLL_SPEED
    STORE16 R0, SCROLL_Y_VAR

CHECK_KEY_LEFT:
    MOV   R0, R3
    LOADI R1, KEY_LEFT
    AND   R0, R1
    JZ    CHECK_KEY_RIGHT
    LOAD16 R0, SCROLL_X_VAR
    SUBI  R0, SCROLL_SPEED
    STORE16 R0, SCROLL_X_VAR

CHECK_KEY_RIGHT:
    MOV   R0, R3
    LOADI R1, KEY_RIGHT
    AND   R0, R1
    JZ    KEYS_DONE
    LOAD16 R0, SCROLL_X_VAR
    ADDI  R0, SCROLL_SPEED
    STORE16 R0, SCROLL_X_VAR

KEYS_DONE:
    RET

; =============================================================================
; DATA SECTION
; =============================================================================

section .data

ORG USER_DATA_BASE
last_frame: DB 0
scroll_x:   DB 0
scroll_y:   DB 0

; Include tileset (tiles and palette)
.include "../includes/tileset.inc"
