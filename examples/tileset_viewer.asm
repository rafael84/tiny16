.include "../stdlib/tiny16.inc"

; Memory addresses
USER_DATA_BASE     = 0x4000
TILE_OFFSET_ADDR   = USER_DATA_BASE + 0
FRAME_COUNTER_ADDR = USER_DATA_BASE + 1
LAST_FRAME_ADDR    = USER_DATA_BASE + 2

TILES_PER_SCREEN   = 128          ; 16×8 = 128 tiles visible
MAX_TILE_INDEX     = 80           ; Total tiles in tileset
SCROLL_SPEED       = 8            ; Frames between scrolls

section .code

START:
    ; Initialize tile offset to 0
    LOADI R6, TILE_OFFSET_ADDR >> 8
    LOADI R7, TILE_OFFSET_ADDR & 0xFF
    LOADI R0, 0
    STORE R0, [R6:R7]

    LOADI R6, FRAME_COUNTER_ADDR >> 8
    LOADI R7, FRAME_COUNTER_ADDR & 0xFF
    LOADI R0, 0
    STORE R0, [R6:R7]

    ; Draw initial tilemap
    CALL DRAW_TILEMAP

    ; Enable rendering
    LOADI R6, 0xBF
    LOADI R7, 0x30
    LOADI R0, 0x01
    STORE R0, [R6:R7]

MAIN_LOOP:
    ; Wait for next frame
    CALL WAIT_VSYNC

    ; Increment frame counter
    LOADI R6, FRAME_COUNTER_ADDR >> 8
    LOADI R7, FRAME_COUNTER_ADDR & 0xFF
    LOAD  R0, [R6:R7]
    INC   R0
    STORE R0, [R6:R7]

    ; Check if it's time to scroll (every SCROLL_SPEED frames)
    LOADI R1, SCROLL_SPEED
    CALL  MOD_R0_BY_R1
    CMPI  R0, 0
    JNZ   RENDER_ONLY      ; Not time to scroll yet

    ; Time to scroll: increment tile offset
    LOADI R6, TILE_OFFSET_ADDR >> 8
    LOADI R7, TILE_OFFSET_ADDR & 0xFF
    LOAD  R0, [R6:R7]
    ADDI  R0, 16           ; Scroll by one row (16 tiles)

    ; Wrap around if we exceed max tiles
    CMPI  R0, MAX_TILE_INDEX
    JC    STORE_OFFSET     ; R0 < MAX_TILE_INDEX, no wrap
    LOADI R0, 0            ; Wrap to beginning

STORE_OFFSET:
    LOADI R6, TILE_OFFSET_ADDR >> 8
    LOADI R7, TILE_OFFSET_ADDR & 0xFF
    STORE R0, [R6:R7]
    CALL  DRAW_TILEMAP

RENDER_ONLY:
    ; Trigger PPU render
    LOADI R6, 0xBF
    LOADI R7, 0x30
    LOADI R0, 0x81
    STORE R0, [R6:R7]

    JMP MAIN_LOOP

; =============================================================================
; DRAW_TILEMAP - Draw 16×8 grid of tiles starting from tile_offset
; =============================================================================
DRAW_TILEMAP:
    ; Starting tilemap position: X=0, Y=4 (centered vertically)
    ; Starting address = 0x7000 + (4 * 32 + 0) * 2 = 0x7100
    LOADI R6, 0x71        ; High byte of start address
    LOADI R7, 0x00        ; Low byte of start address

    ; Load starting tile index
    PUSH  R6
    PUSH  R7
    LOADI R6, TILE_OFFSET_ADDR >> 8
    LOADI R7, TILE_OFFSET_ADDR & 0xFF
    LOAD  R3, [R6:R7]     ; Starting tile index
    POP   R7
    POP   R6

    LOADI R4, 8           ; Row counter (8 rows)

DRAW_ROW_LOOP:
    LOADI R2, 16          ; Column counter (16 tiles per row)
DRAW_COL_LOOP:
    MOV   R0, R3          ; Current tile index
    STORE R0, [R6:R7]+    ; Store tile index, increment address
    LOADI R0, 0           ; Tile attribute (0)
    STORE R0, [R6:R7]+    ; Store attribute, increment address
    INC   R3              ; Next tile

    ; Check if we've exceeded max tiles, wrap to 0
    CMPI  R3, MAX_TILE_INDEX
    JC    DRAW_NO_WRAP
    LOADI R3, 0           ; Wrap to tile 0
DRAW_NO_WRAP:

    DEC   R2              ; Decrement column counter
    JNZ   DRAW_COL_LOOP   ; Continue until 16 tiles done

    ; Skip to next row: add (32-16)*2 = 32 bytes to address
    LOADI R0, 32
    ADD   R7, R0          ; Add 32 to low byte
    LOADI R0, 0
    ADC   R6, R0          ; Add carry to high byte

    DEC   R4              ; Decrement row counter
    JNZ   DRAW_ROW_LOOP   ; Continue until 8 rows done

    RET

; =============================================================================
; MOD_R0_BY_R1 - R0 = R0 % R1 (modulo operation)
; =============================================================================
MOD_R0_BY_R1:
    CMP   R0, R1
    JC    MOD_DONE
    SUB   R0, R1
    JMP   MOD_R0_BY_R1
MOD_DONE:
    RET

; =============================================================================
; WAIT_VSYNC - Wait for frame counter to change
; =============================================================================
WAIT_VSYNC:
    PUSH  R0
    PUSH  R1
    PUSH  R6
    PUSH  R7
    LOADI R6, 0xBF
    LOADI R7, 0x22
    LOAD  R0, [R6:R7]
VSYNC_WAIT_LOOP:
    LOAD  R1, [R6:R7]
    CMP   R0, R1
    JZ    VSYNC_WAIT_LOOP
    POP   R7
    POP   R6
    POP   R1
    POP   R0
    RET

section .data

; User data storage
ORG USER_DATA_BASE
tile_offset:    DB 0    ; Current tile offset for scrolling
frame_counter:  DB 0    ; Frame counter for timing
last_frame:     DB 0    ; Last frame value for vsync

.include "tileset.inc"
