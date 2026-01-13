; Bouncing Smileys Demo - 32 PPU Sprites
;
; This demo demonstrates:
; - PPU tile-based rendering
; - Hardware sprites via OAM (32 bouncing smileys)
; - Using data section for constants and arrays
; - Animation synchronized to display frame rate
;
; Memory layout:
; 0x4000-0x4001: initialized flag, last_frame
; 0x4010-0x408F: sprite data (32 sprites × 4 bytes each)
;                [x, y, vel_x, vel_y] per sprite
; 0x5000: Tile data
; 0x7000: Tilemap
; 0x7800: OAM (64 entries × 4 bytes)
; 0x7900: Palette
; 0xBF30: PPU_CTRL
;

section .code

START:
    ; Check if already initialized
    LOADI R6, 0x40
    LOADI R7, 0x00    ; initialized flag at 0x4000
    LOAD  R0
    LOADI R1, 0xAA
    CMP   R0, R1
    JZ    MAIN_LOOP

    CALL  INIT_PALETTE
    CALL  INIT_TILES
    CALL  INIT_OAM
    CALL  INIT_SPRITES

MAIN_LOOP:
    CALL  WAIT_FRAME
    CALL  UPDATE_ALL_OAM
    CALL  UPDATE_ALL_POSITIONS
    CALL  RENDER_FRAME
    JMP   MAIN_LOOP

; =============================================================================
; INIT_PALETTE - Set up palette colors at 0x7900
; =============================================================================
INIT_PALETTE:
    LOADI R6, 0x79    ; PALETTE_BASE high
    LOADI R7, 0x00    ; PALETTE_BASE low

    ; Color 0: dark blue background (0x03)
    LOADI R0, 0x03
    STORE R0
    INC   R7
    LOADI R0, 0x00    ; padding
    STORE R0
    INC   R7

    ; Color 1: blue border (0x1F)
    LOADI R0, 0x1F
    STORE R0
    INC   R7
    LOADI R0, 0x00
    STORE R0
    INC   R7

    ; Color 2: yellow face (0xFC)
    LOADI R0, 0xFC
    STORE R0
    INC   R7
    LOADI R0, 0x00
    STORE R0
    INC   R7

    ; Color 3: black (eyes/mouth) (0x00)
    LOADI R0, 0x00
    STORE R0
    INC   R7
    STORE R0
    RET

; =============================================================================
; INIT_TILES - Set up smiley tile at index 0
; Tile format: 4bpp, 8x8 pixels = 32 bytes
; =============================================================================
INIT_TILES:
    LOADI R6, 0x50    ; TILES_BASE high
    LOADI R7, 0x00    ; TILES_BASE low

    ; Row 0: 0 1 1 1 1 1 1 0 -> 0x01 0x11 0x11 0x10
    LOADI R0, 0x01
    STORE R0
    INC   R7
    LOADI R0, 0x11
    STORE R0
    INC   R7
    STORE R0
    INC   R7
    LOADI R0, 0x10
    STORE R0
    INC   R7

    ; Row 1: 1 2 2 2 2 2 2 1 -> 0x12 0x22 0x22 0x21
    LOADI R0, 0x12
    STORE R0
    INC   R7
    LOADI R0, 0x22
    STORE R0
    INC   R7
    STORE R0
    INC   R7
    LOADI R0, 0x21
    STORE R0
    INC   R7

    ; Row 2: 1 2 3 2 2 3 2 1 -> 0x12 0x32 0x23 0x21
    LOADI R0, 0x12
    STORE R0
    INC   R7
    LOADI R0, 0x32
    STORE R0
    INC   R7
    LOADI R0, 0x23
    STORE R0
    INC   R7
    LOADI R0, 0x21
    STORE R0
    INC   R7

    ; Row 3: 1 2 2 2 2 2 2 1 -> 0x12 0x22 0x22 0x21
    LOADI R0, 0x12
    STORE R0
    INC   R7
    LOADI R0, 0x22
    STORE R0
    INC   R7
    STORE R0
    INC   R7
    LOADI R0, 0x21
    STORE R0
    INC   R7

    ; Row 4: 1 2 3 2 2 3 2 1 -> 0x12 0x32 0x23 0x21
    LOADI R0, 0x12
    STORE R0
    INC   R7
    LOADI R0, 0x32
    STORE R0
    INC   R7
    LOADI R0, 0x23
    STORE R0
    INC   R7
    LOADI R0, 0x21
    STORE R0
    INC   R7

    ; Row 5: 1 2 2 3 3 2 2 1 -> 0x12 0x23 0x32 0x21
    LOADI R0, 0x12
    STORE R0
    INC   R7
    LOADI R0, 0x23
    STORE R0
    INC   R7
    LOADI R0, 0x32
    STORE R0
    INC   R7
    LOADI R0, 0x21
    STORE R0
    INC   R7

    ; Row 6: 1 2 2 2 2 2 2 1 -> 0x12 0x22 0x22 0x21
    LOADI R0, 0x12
    STORE R0
    INC   R7
    LOADI R0, 0x22
    STORE R0
    INC   R7
    STORE R0
    INC   R7
    LOADI R0, 0x21
    STORE R0
    INC   R7

    ; Row 7: 0 1 1 1 1 1 1 0 -> 0x01 0x11 0x11 0x10
    LOADI R0, 0x01
    STORE R0
    INC   R7
    LOADI R0, 0x11
    STORE R0
    INC   R7
    STORE R0
    INC   R7
    LOADI R0, 0x10
    STORE R0
    RET

; =============================================================================
; INIT_OAM - Hide all 64 sprites by setting Y to 0xFF
; =============================================================================
INIT_OAM:
    LOADI R6, 0x78    ; OAM_BASE high
    LOADI R7, 0x00    ; OAM_BASE low
    LOADI R0, 0xFF    ; Y = 0xFF means hidden
    LOADI R1, 64      ; 64 sprites

INIT_OAM_LOOP:
    STORE R0          ; Y = 0xFF (hidden)
    INC   R7
    INC   R7
    INC   R7
    INC   R7          ; Next OAM entry (4 bytes each)
    DEC   R1
    JNZ   INIT_OAM_LOOP
    RET

; =============================================================================
; INIT_SPRITES - Initialize 32 sprites with varied positions and velocities
; Sprite data at 0x4010: [x, y, vel_x, vel_y] × 32
; Uses a simple pattern to distribute sprites
; =============================================================================
INIT_SPRITES:
    LOADI R6, 0x40
    LOADI R7, 0x10    ; sprite data starts at 0x4010
    LOADI R4, 0       ; sprite counter (0-31)
    LOADI R5, 32      ; total sprites

INIT_SPRITE_LOOP:
    ; X = (counter * 4) mod 120
    MOV   R0, R4
    ADD   R0, R0      ; R0 = counter * 2
    ADD   R0, R0      ; R0 = counter * 4
    ; Keep X in range 0-119 (simple approach: just use lower bits)
    STORE R0          ; x
    INC   R7

    ; Y = (counter * 3 + 10) mod 120
    MOV   R1, R4
    ADD   R1, R4      ; R1 = counter * 2
    ADD   R1, R4      ; R1 = counter * 3
    LOADI R2, 10
    ADD   R1, R2      ; R1 = counter * 3 + 10
    STORE R1          ; y
    INC   R7

    ; vel_x = counter & 1 (alternating 0/1)
    MOV   R0, R4
    LOADI R2, 0x01
    AND   R0, R2
    STORE R0          ; vel_x
    INC   R7

    ; vel_y = (counter >> 1) & 1 (alternating pairs)
    MOV   R0, R4
    LOADI R2, 0x02
    AND   R0, R2
    ; Shift right by 1 (divide by 2) - need to check if bit 1 is set
    JZ    VEL_Y_ZERO
    LOADI R0, 1
    JMP   STORE_VEL_Y
VEL_Y_ZERO:
    LOADI R0, 0
STORE_VEL_Y:
    STORE R0          ; vel_y
    INC   R7

    INC   R4          ; next sprite
    DEC   R5
    JNZ   INIT_SPRITE_LOOP

    ; Mark as initialized
    LOADI R6, 0x40
    LOADI R7, 0x00
    LOADI R0, 0xAA
    STORE R0          ; initialized flag

    RET

; =============================================================================
; UPDATE_ALL_OAM - Update OAM for all 32 sprites
; =============================================================================
UPDATE_ALL_OAM:
    LOADI R4, 0       ; sprite counter
    LOADI R5, 32      ; total sprites

UPDATE_OAM_LOOP:
    ; Calculate sprite data address: 0x4010 + (counter * 4)
    LOADI R6, 0x40
    MOV   R7, R4
    ADD   R7, R7      ; counter * 2
    ADD   R7, R7      ; counter * 4
    LOADI R0, 0x10
    ADD   R7, R0      ; R7 = 0x10 + counter * 4

    ; Load x, y
    LOAD  R0          ; x
    INC   R7
    LOAD  R1          ; y

    ; Calculate OAM address: 0x7800 + (counter * 4)
    LOADI R6, 0x78
    MOV   R7, R4
    ADD   R7, R7      ; counter * 2
    ADD   R7, R7      ; counter * 4

    ; Write OAM entry: Y, X, tile, attr
    STORE R1          ; Y position
    INC   R7
    STORE R0          ; X position
    INC   R7
    LOADI R0, 0x00    ; Tile index 0 (smiley)
    STORE R0
    INC   R7
    LOADI R0, 0x00    ; Attributes (no flip)
    STORE R0

    INC   R4          ; next sprite
    DEC   R5
    JNZ   UPDATE_OAM_LOOP
    RET

; =============================================================================
; UPDATE_ALL_POSITIONS - Update positions for all 32 sprites
; =============================================================================
UPDATE_ALL_POSITIONS:
    LOADI R4, 0       ; sprite counter
    LOADI R5, 32      ; total sprites

UPDATE_POS_LOOP:
    PUSH  R4          ; save counter
    PUSH  R5          ; save remaining

    ; Calculate sprite data address: 0x4010 + (counter * 4)
    LOADI R6, 0x40
    MOV   R7, R4
    ADD   R7, R7      ; counter * 2
    ADD   R7, R7      ; counter * 4
    LOADI R0, 0x10
    ADD   R7, R0      ; R7 = 0x10 + counter * 4

    ; Load x, y, vel_x, vel_y
    LOAD  R0          ; x
    INC   R7
    LOAD  R1          ; y
    INC   R7
    LOAD  R2          ; vel_x
    INC   R7
    LOAD  R3          ; vel_y

    ; Save base address for later
    DEC   R7
    DEC   R7
    DEC   R7          ; back to x address
    PUSH  R7          ; save address low byte

    ; Update X position
    LOADI R5, 0
    CMP   R2, R5
    JZ    POS_MOVE_LEFT

    ; Moving right
    LOADI R5, 120     ; boundary
    CMP   R0, R5
    JZ    POS_BOUNCE_X
    INC   R0
    JMP   POS_UPDATE_Y

POS_MOVE_LEFT:
    LOADI R5, 0
    CMP   R0, R5
    JZ    POS_BOUNCE_X
    DEC   R0
    JMP   POS_UPDATE_Y

POS_BOUNCE_X:
    LOADI R5, 1
    SUB   R5, R2
    MOV   R2, R5

POS_UPDATE_Y:
    LOADI R5, 0
    CMP   R3, R5
    JZ    POS_MOVE_UP

    ; Moving down
    LOADI R5, 120     ; boundary
    CMP   R1, R5
    JZ    POS_BOUNCE_Y
    INC   R1
    JMP   POS_SAVE

POS_MOVE_UP:
    LOADI R5, 0
    CMP   R1, R5
    JZ    POS_BOUNCE_Y
    DEC   R1
    JMP   POS_SAVE

POS_BOUNCE_Y:
    LOADI R5, 1
    SUB   R5, R3
    MOV   R3, R5

POS_SAVE:
    ; Restore address and save position
    POP   R7          ; restore address low byte
    LOADI R6, 0x40
    STORE R0          ; x
    INC   R7
    STORE R1          ; y
    INC   R7
    STORE R2          ; vel_x
    INC   R7
    STORE R3          ; vel_y

    POP   R5          ; restore remaining count
    POP   R4          ; restore sprite counter
    INC   R4          ; next sprite
    DEC   R5
    JNZ   UPDATE_POS_LOOP
    RET

; =============================================================================
; RENDER_FRAME - Trigger PPU to render
; =============================================================================
RENDER_FRAME:
    LOADI R6, 0xBF    ; PPU_CTRL high
    LOADI R7, 0x30    ; PPU_CTRL low
    ; Enable sprites + RENDER_NOW = 0x02 | 0x80 = 0x82 (no background)
    LOADI R0, 0x82
    STORE R0
    RET

; =============================================================================
; WAIT_FRAME - Wait for FRAME_COUNT to change (syncs to 60 FPS)
; =============================================================================
WAIT_FRAME:
    ; Read current FRAME_COUNT
    LOADI R6, 0xBF
    LOADI R7, 0x22    ; FRAME_COUNT at 0xBF22
    LOAD  R0          ; R0 = current frame count

    ; Load last frame count
    LOADI R6, 0x40
    LOADI R7, 0x01    ; last_frame at 0x4001
    LOAD  R1          ; R1 = last frame count

    ; Compare
    CMP   R0, R1
    JZ    WAIT_FRAME  ; If same, keep waiting

    ; Frame changed - save new frame count
    STORE R0
    RET

section .data

; State at 0x4000
initialized:  DB 0     ; 0x4000 - initialization flag (0xAA when done)
last_frame:   DB 0     ; 0x4001 - last known frame count

; Padding to align sprite data at 0x4010
              DB 0,0,0,0,0,0,0,0,0,0,0,0,0,0  ; 14 bytes padding (0x4002-0x400F)

; Sprite data array (32 sprites × 4 bytes = 128 bytes)
; Each sprite: [x, y, vel_x, vel_y]
; Starts at 0x4010, ends at 0x408F
sprite_data:  DB 0     ; Will be filled by INIT_SPRITES
