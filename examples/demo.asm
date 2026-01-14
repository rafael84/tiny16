; Bouncing Smileys Demo - 32 PPU Sprites
;
; This demo demonstrates:
; - PPU tile-based rendering with data-defined tiles
; - Hardware sprites via OAM (32 bouncing smileys)
; - Using TIMES to position data at specific memory addresses
; - Animation synchronized to display frame rate
; - New addressing modes: post-increment [PAIR]+ and offset [PAIR + imm8]
;
; Memory layout (all defined in data section):
; 0x4000-0x4001: initialized flag, last_frame
; 0x4010-0x408F: sprite data (32 sprites × 4 bytes each)
; 0x5000-0x501F: Tile 0 (smiley face, 32 bytes)
; 0x7800-0x78FF: OAM (64 entries × 4 bytes, hidden by default)
; 0x7900-0x7907: Palette (4 colors × 2 bytes)
;

section .code

START:
    ; Check if already initialized
    LOADI R6, 0x40
    LOADI R7, 0x00    ; initialized flag at 0x4000
    LOAD  R0, [R6:R7]
    LOADI R1, 0xAA
    CMP   R0, R1
    JZ    MAIN_LOOP

    ; Initialize sprite positions (computed at runtime)
    CALL  INIT_SPRITES

MAIN_LOOP:
    CALL  WAIT_FRAME
    CALL  UPDATE_ALL_OAM
    CALL  UPDATE_ALL_POSITIONS
    CALL  RENDER_FRAME
    JMP   MAIN_LOOP

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
    ; X = (counter * 4)
    MOV   R0, R4
    TIMES 2 ADD R0, R0    ; R0 = counter * 4
    STORE R0, [R6:R7]+         ; x (post-increment)

    ; Y = (counter * 3 + 10)
    MOV   R1, R4
    ADD   R1, R4      ; R1 = counter * 2
    ADD   R1, R4      ; R1 = counter * 3
    LOADI R2, 10
    ADD   R1, R2      ; R1 = counter * 3 + 10
    STORE R1, [R6:R7]+         ; y (post-increment)

    ; vel_x = 2 or 3 based on counter & 1 (faster velocity!)
    MOV   R0, R4
    LOADI R2, 0x01
    AND   R0, R2
    LOADI R1, 2
    ADD   R0, R1      ; vel_x = 2 or 3
    STORE R0, [R6:R7]+         ; vel_x (post-increment)

    ; vel_y = 2 or 3 based on (counter >> 1) & 1
    MOV   R0, R4
    LOADI R2, 0x02
    AND   R0, R2
    JZ    VEL_Y_LOW
    LOADI R0, 3
    JMP   STORE_VEL_Y
VEL_Y_LOW:
    LOADI R0, 2
STORE_VEL_Y:
    STORE R0, [R6:R7]+         ; vel_y (post-increment)

    INC   R4          ; next sprite
    DEC   R5
    JNZ   INIT_SPRITE_LOOP

    ; Mark as initialized
    LOADI R6, 0x40
    LOADI R7, 0x00
    LOADI R0, 0xAA
    STORE R0, [R6:R7]          ; initialized flag

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
    TIMES 2 ADD R7, R7    ; counter * 4
    LOADI R0, 0x10
    ADD   R7, R0      ; R7 = 0x10 + counter * 4

    ; Load x, y using offset addressing (no pointer modification needed)
    LOAD  R0, [R6:R7 + 0]      ; x at offset 0
    LOAD  R1, [R6:R7 + 1]      ; y at offset 1

    ; Calculate OAM address: 0x7800 + (counter * 4)
    LOADI R6, 0x78
    MOV   R7, R4
    TIMES 2 ADD R7, R7    ; counter * 4

    ; Write OAM entry using post-increment: Y, X, tile, attr
    STORE R1, [R6:R7]+         ; Y position
    STORE R0, [R6:R7]+         ; X position
    LOADI R0, 0x00    ; Tile index 0 (smiley)
    STORE R0, [R6:R7]+
    LOADI R0, 0x00    ; Attributes (no flip)
    STORE R0, [R6:R7]

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
    TIMES 2 ADD R7, R7    ; counter * 4
    LOADI R0, 0x10
    ADD   R7, R0      ; R7 = 0x10 + counter * 4

    ; Load x, y, vel_x, vel_y using offset addressing
    LOAD  R0, [R6:R7 + 0]      ; x
    LOAD  R1, [R6:R7 + 1]      ; y
    LOAD  R2, [R6:R7 + 2]      ; vel_x (now 2-3)
    LOAD  R3, [R6:R7 + 3]      ; vel_y (now 2-3)

    PUSH  R7          ; save address low byte

    ; Update X position: add velocity, check bounds
    ; vel_x > 1 means moving right, vel_x = 0 means moving left
    LOADI R5, 1
    CMP   R2, R5
    JC    POS_MOVE_LEFT        ; vel_x < 1 means moving left (vel_x = 0)
    JZ    POS_MOVE_LEFT        ; vel_x == 1 also left

    ; Moving right: x += vel_x
    ADD   R0, R2
    LOADI R5, 120     ; boundary
    CMP   R0, R5
    JC    POS_UPDATE_Y         ; x < 120, no bounce
    ; Bounce: clamp x and reverse velocity
    MOV   R0, R5               ; x = 120
    XOR   R2, R2               ; vel_x = 0 (move left)
    JMP   POS_UPDATE_Y

POS_MOVE_LEFT:
    ; Moving left: x -= speed (speed stored when we reversed)
    ; For left movement, vel_x is 0, we use a fixed speed of 2
    LOADI R5, 2
    CMP   R0, R5
    JC    POS_BOUNCE_X_LEFT    ; x < 2, will underflow
    SUB   R0, R5
    JMP   POS_UPDATE_Y

POS_BOUNCE_X_LEFT:
    XOR   R0, R0               ; x = 0
    LOADI R2, 2                ; vel_x = 2 (move right)

POS_UPDATE_Y:
    ; Update Y position: similar logic
    LOADI R5, 1
    CMP   R3, R5
    JC    POS_MOVE_UP          ; vel_y < 1 means moving up
    JZ    POS_MOVE_UP          ; vel_y == 1 also up

    ; Moving down: y += vel_y
    ADD   R1, R3
    LOADI R5, 120     ; boundary
    CMP   R1, R5
    JC    POS_SAVE             ; y < 120, no bounce
    ; Bounce: clamp y and reverse velocity
    MOV   R1, R5               ; y = 120
    XOR   R3, R3               ; vel_y = 0 (move up)
    JMP   POS_SAVE

POS_MOVE_UP:
    ; Moving up: y -= speed
    LOADI R5, 2
    CMP   R1, R5
    JC    POS_BOUNCE_Y_UP      ; y < 2, will underflow
    SUB   R1, R5
    JMP   POS_SAVE

POS_BOUNCE_Y_UP:
    XOR   R1, R1               ; y = 0
    LOADI R3, 2                ; vel_y = 2 (move down)

POS_SAVE:
    ; Restore address and save position using post-increment
    POP   R7          ; restore address low byte
    LOADI R6, 0x40
    STORE R0, [R6:R7]+         ; x
    STORE R1, [R6:R7]+         ; y
    STORE R2, [R6:R7]+         ; vel_x
    STORE R3, [R6:R7]          ; vel_y

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
    STORE R0, [R6:R7]
    RET

; =============================================================================
; WAIT_FRAME - Wait for FRAME_COUNT to change (syncs to 60 FPS)
; =============================================================================
WAIT_FRAME:
    ; Read current FRAME_COUNT
    LOADI R6, 0xBF
    LOADI R7, 0x22    ; FRAME_COUNT at 0xBF22
    LOAD  R0, [R6:R7]          ; R0 = current frame count

    ; Load last frame count
    LOADI R6, 0x40
    LOADI R7, 0x01    ; last_frame at 0x4001
    LOAD  R1, [R6:R7]          ; R1 = last frame count

    ; Compare
    CMP   R0, R1
    JZ    WAIT_FRAME  ; If same, keep waiting

    ; Frame changed - save new frame count
    STORE R0, [R6:R7]
    RET

; =============================================================================
; DATA SECTION - All graphics data defined here using TIMES for positioning
; =============================================================================

section .data

; ============================================================================
; User Data (0x4000 - 0x408F)
; ============================================================================

; State at 0x4000
initialized:  DB 0                ; 0x4000 - initialization flag (0xAA when done)
last_frame:   DB 0                ; 0x4001 - last known frame count

; Padding to align sprite data at 0x4010
              TIMES 14 DB 0       ; 0x4002-0x400F

; Sprite data array (32 sprites × 4 bytes = 128 bytes)
; Each sprite: [x, y, vel_x, vel_y]
sprite_data:  TIMES 128 DB 0      ; 0x4010-0x408F

; ============================================================================
; Padding to reach Tile Memory (0x5000)
; From 0x4090 to 0x4FFF = 0x0F70 = 3952 bytes
; ============================================================================
              TIMES 3952 DB 0

; ============================================================================
; Tile 0: Smiley Face at 0x5000 (32 bytes)
; 8x8 pixels, 4bpp format (2 pixels per byte)
;
; Pixel values: 0=transparent, 1=border(blue), 2=face(yellow), 3=eyes(black)
;
;   01111110    Row 0
;   12222221    Row 1
;   12322321    Row 2 (eyes)
;   12222221    Row 3
;   12322321    Row 4 (eyes)
;   12233221    Row 5 (mouth)
;   12222221    Row 6
;   01111110    Row 7
; ============================================================================
tile_smiley:
    ; Row 0: 0 1 1 1 1 1 1 0
    DB 0x01, 0x11, 0x11, 0x10
    ; Row 1: 1 2 2 2 2 2 2 1
    DB 0x12, 0x22, 0x22, 0x21
    ; Row 2: 1 2 3 2 2 3 2 1 (eyes)
    DB 0x12, 0x32, 0x23, 0x21
    ; Row 3: 1 2 2 2 2 2 2 1
    DB 0x12, 0x22, 0x22, 0x21
    ; Row 4: 1 2 3 2 2 3 2 1 (eyes)
    DB 0x12, 0x32, 0x23, 0x21
    ; Row 5: 1 2 2 3 3 2 2 1 (mouth)
    DB 0x12, 0x23, 0x32, 0x21
    ; Row 6: 1 2 2 2 2 2 2 1
    DB 0x12, 0x22, 0x22, 0x21
    ; Row 7: 0 1 1 1 1 1 1 0
    DB 0x01, 0x11, 0x11, 0x10

; ============================================================================
; Padding to reach OAM (0x7800)
; From 0x5020 to 0x77FF = 0x27E0 = 10208 bytes
; ============================================================================
              TIMES 10208 DB 0

; ============================================================================
; OAM at 0x7800 (64 entries × 4 bytes = 256 bytes)
; Format: [Y, X, tile, attr] - Y=0xFF means hidden
; All 64 sprites start hidden (Y = 0xFF)
; ============================================================================
oam_data:
    ; Each OAM entry: Y=0xFF (hidden), X=0, tile=0, attr=0
    TIMES 64 DB 0xFF, 0x00, 0x00, 0x00

; ============================================================================
; Palette at 0x7900 (16 colors × 2 bytes = 32 bytes, using first 4)
; Format: [color_RGB332, padding]
; ============================================================================
palette:
    ; Color 0: dark blue background (RGB332: 0x03)
    DB 0x03, 0x00
    ; Color 1: blue border (RGB332: 0x1F)
    DB 0x1F, 0x00
    ; Color 2: yellow face (RGB332: 0xFC)
    DB 0xFC, 0x00
    ; Color 3: black eyes/mouth (RGB332: 0x00)
    DB 0x00, 0x00
