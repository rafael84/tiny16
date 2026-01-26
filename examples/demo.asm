; Bouncing Smileys Demo - 32 PPU Sprites
;
; This demo demonstrates:
; - PPU tile-based rendering with data-defined tiles
; - Hardware sprites via OAM (32 bouncing smileys)
; - Using ORG to position data at specific memory addresses
; - Using expressions in constants for computed values
; - Animation synchronized to display frame rate
; - New addressing modes: post-increment [PAIR]+ and offset [PAIR + imm8]
;
; Memory layout (all defined in data section):
; USER_DATA_BASE+0:  initialized flag
; USER_DATA_BASE+1:  last_frame counter
; USER_DATA_BASE+16: sprite data (SPRITE_COUNT × SPRITE_SIZE bytes)
; TILE_DATA_ADDR:    Tile 0 (smiley, 32 bytes)
; OAM_ADDR:          OAM (64 entries × 4 bytes)
; PALETTE_ADDR:      Palette (16 colors × 2 bytes)
;

.include "../stdlib/tiny16.inc"

; =============================================================================
; Constants - Memory Map
; =============================================================================

; Data section base addresses
USER_DATA_BASE     = 0x4000
TILE_DATA_ADDR     = 0x5000
OAM_ADDR           = 0x7800
PALETTE_ADDR       = 0x7900

; Data layout offsets
INITIALIZED_ADDR   = USER_DATA_BASE + 0
LAST_FRAME_ADDR    = USER_DATA_BASE + 1
SPRITE_DATA_ADDR   = USER_DATA_BASE + 16

; Address bytes used in tight loops (computed from full addresses)
INITIALIZED_HI     = INITIALIZED_ADDR >> 8
INITIALIZED_LO     = INITIALIZED_ADDR & 0xFF
LAST_FRAME_HI      = LAST_FRAME_ADDR >> 8
LAST_FRAME_LO      = LAST_FRAME_ADDR & 0xFF
SPRITE_DATA_HI     = SPRITE_DATA_ADDR >> 8
SPRITE_DATA_LO     = SPRITE_DATA_ADDR & 0xFF
OAM_HI             = OAM_ADDR >> 8
OAM_LO             = OAM_ADDR & 0xFF

; =============================================================================
; Constants - Game Parameters
; =============================================================================

SPRITE_COUNT       = 32
SPRITE_SIZE        = 4
SPRITE_DATA_SIZE   = SPRITE_COUNT * SPRITE_SIZE
BOUNDARY           = 120
MIN_POSITION       = 10
MODULO_RANGE       = BOUNDARY - MIN_POSITION
VELOCITY_BASE      = 4
INITIALIZED_FLAG   = 0xAA

TILE_SMILEY        = 0x00
SPRITE_ATTR_NONE   = 0x00

section .code

START:
    ; Check if already initialized
    LOAD16 R0, INITIALIZED_HI, INITIALIZED_LO
    LOADI R1, INITIALIZED_FLAG
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
; Uses frame counter as pseudo-random seed for more varied distribution
; =============================================================================
INIT_SPRITES:
    ; Read FRAME_COUNT for pseudo-random seed
    PUSH2 R6, R7
    READ_FRAME_COUNT R3            ; R3 = frame count (pseudo-random seed)
    POP2  R6, R7

    SETADDR SPRITE_DATA_HI, SPRITE_DATA_LO
    LOADI R4, 0       ; sprite counter (0-31)
    LOADI R5, SPRITE_COUNT

INIT_SPRITE_LOOP:
    ; X = ((seed + counter * 7) * 3 + 10) % 110 (range 10-119)
    MOV   R0, R4
    TIMES 7 ADD R0, R4         ; counter * 7
    ADD   R0, R3               ; add seed
    ADD   R0, R0               ; * 2
    ADD   R0, R0               ; * 2 (total * 4 from counter*7)
    ; Modulo 110 to keep in range, then add 10
    LOADI R2, MODULO_RANGE
    CALL  MOD_R0_BY_R2
    LOADI R2, MIN_POSITION
    ADD   R0, R2               ; R0 = x position (10-119)
    STORE R0, [R6:R7]+         ; x (post-increment)

    ; Y = ((seed + counter * 11) * 2 + 10) % 110 (range 10-119)
    MOV   R1, R4
    TIMES 11 ADD R1, R4        ; counter * 11
    ADD   R1, R3               ; add seed
    ADD   R1, R1               ; * 2
    ; Modulo 110 to keep in range, then add 10
    MOV   R0, R1               ; prepare for modulo
    LOADI R2, MODULO_RANGE
    CALL  MOD_R0_BY_R2
    LOADI R2, MIN_POSITION
    ADD   R0, R2               ; R0 = y position (10-119)
    MOV   R1, R0               ; restore to R1
    STORE R1, [R6:R7]+         ; y (post-increment)

    ; vel_x = 4 or 5 based on (seed + counter) & 1
    MOV   R0, R3
    ADD   R0, R4
    LOADI R2, 0x01
    AND   R0, R2
    LOADI R1, VELOCITY_BASE
    ADD   R0, R1               ; vel_x = 4 or 5
    STORE R0, [R6:R7]+         ; vel_x (post-increment)

    ; vel_y = 4 or 5 based on (seed + counter * 3) & 1
    MOV   R0, R4
    TIMES 3 ADD R0, R4         ; counter * 3
    ADD   R0, R3               ; add seed
    LOADI R2, 0x01
    AND   R0, R2
    LOADI R1, VELOCITY_BASE
    ADD   R0, R1               ; vel_y = 4 or 5
    STORE R0, [R6:R7]+         ; vel_y (post-increment)

    INC   R4          ; next sprite
    DEC   R5
    JNZ   INIT_SPRITE_LOOP

    ; Mark as initialized
    STORE16I INITIALIZED_FLAG, INITIALIZED_HI, INITIALIZED_LO

    RET

; =============================================================================
; MOD_R0_BY_R2 - R0 = R0 % R2 (modulo operation)
; Input: R0 = dividend, R2 = divisor
; Output: R0 = remainder
; Clobbers: R0
; =============================================================================
MOD_R0_BY_R2:
    CMP   R0, R2
    JC    MOD_DONE             ; if R0 < R2, done
    SUB   R0, R2
    JMP   MOD_R0_BY_R2         ; repeat
MOD_DONE:
    RET

; =============================================================================
; UPDATE_ALL_OAM - Update OAM for all 32 sprites
; =============================================================================
UPDATE_ALL_OAM:
    LOADI R4, 0       ; sprite counter
    LOADI R5, SPRITE_COUNT

UPDATE_OAM_LOOP:
    ; Calculate sprite data address: SPRITE_DATA_ADDR + (counter * SPRITE_SIZE)
    LOADI R6, SPRITE_DATA_HI
    MOV   R7, R4
    MUL4  R7                   ; counter * 4
    LOADI R0, SPRITE_DATA_LO
    ADD   R7, R0

    ; Load x, y using offset addressing (no pointer modification needed)
    LOAD  R0, [R6:R7 + 0]      ; x at offset 0
    LOAD  R1, [R6:R7 + 1]      ; y at offset 1

    ; Calculate OAM address: 0x7800 + (counter * 4)
    LOADI R6, OAM_HI
    MOV   R7, R4
    MUL4  R7                   ; counter * 4
    LOADI R2, OAM_LO
    ADD   R7, R2

    ; Write OAM entry: Y, X, tile, attr
    OAM_WRITE_SPRITE R1, R0, TILE_SMILEY, SPRITE_ATTR_NONE

    INC   R4          ; next sprite
    DEC   R5
    JNZ   UPDATE_OAM_LOOP
    RET

; =============================================================================
; UPDATE_ALL_POSITIONS - Update positions for all 32 sprites
; =============================================================================
UPDATE_ALL_POSITIONS:
    LOADI R4, 0       ; sprite counter
    LOADI R5, SPRITE_COUNT

UPDATE_POS_LOOP:
    PUSH  R4          ; save counter
    PUSH  R5          ; save remaining

    ; Calculate sprite data address: SPRITE_DATA_ADDR + (counter * SPRITE_SIZE)
    LOADI R6, SPRITE_DATA_HI
    MOV   R7, R4
    MUL4  R7                   ; counter * 4
    LOADI R0, SPRITE_DATA_LO
    ADD   R7, R0

    ; Load x, y, vel_x, vel_y using offset addressing
    LOAD  R0, [R6:R7 + 0]      ; x
    LOAD  R1, [R6:R7 + 1]      ; y
    LOAD  R2, [R6:R7 + 2]      ; vel_x (now 4-5)
    LOAD  R3, [R6:R7 + 3]      ; vel_y (now 4-5)

    PUSH  R7          ; save address low byte

    ; Update X position: add velocity, check bounds
    ; vel_x > 1 means moving right, vel_x = 0 means moving left
    CMPI R2, 1
    JC    POS_MOVE_LEFT        ; vel_x < 1 means moving left (vel_x = 0)
    JZ    POS_MOVE_LEFT        ; vel_x == 1 also left

    ; Moving right: x += vel_x
    ADD   R0, R2
    CMPI R0, BOUNDARY
    JC    POS_UPDATE_Y         ; x < 120, no bounce
    ; Bounce: clamp x and reverse velocity
    LOADI R0, BOUNDARY         ; x = 120
    CLEAR R2                   ; vel_x = 0 (move left)
    JMP   POS_UPDATE_Y

POS_MOVE_LEFT:
    ; Moving left: x -= speed (speed stored when we reversed)
    ; For left movement, vel_x is 0, we use a fixed speed of 4
    CMPI R0, VELOCITY_BASE
    JC    POS_BOUNCE_X_LEFT    ; x < 4, will underflow
    SUBI R0, VELOCITY_BASE
    JMP   POS_UPDATE_Y

POS_BOUNCE_X_LEFT:
    CLEAR R0                   ; x = 0
    LOADI R2, VELOCITY_BASE    ; vel_x = 4 (move right)

POS_UPDATE_Y:
    ; Update Y position: similar logic
    CMPI R3, 1
    JC    POS_MOVE_UP          ; vel_y < 1 means moving up
    JZ    POS_MOVE_UP          ; vel_y == 1 also up

    ; Moving down: y += vel_y
    ADD   R1, R3
    CMPI R1, BOUNDARY
    JC    POS_SAVE             ; y < 120, no bounce
    ; Bounce: clamp y and reverse velocity
    LOADI R1, BOUNDARY         ; y = 120
    CLEAR R3                   ; vel_y = 0 (move up)
    JMP   POS_SAVE

POS_MOVE_UP:
    ; Moving up: y -= speed
    CMPI R1, VELOCITY_BASE
    JC    POS_BOUNCE_Y_UP      ; y < 4, will underflow
    SUBI R1, VELOCITY_BASE
    JMP   POS_SAVE

POS_BOUNCE_Y_UP:
    CLEAR R1                   ; y = 0
    LOADI R3, VELOCITY_BASE    ; vel_y = 4 (move down)

POS_SAVE:
    ; Restore address and save position using post-increment
    POP   R7          ; restore address low byte
    LOADI R6, SPRITE_DATA_HI
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
    PPU_SPRITES_ON
    RET

; =============================================================================
; WAIT_FRAME - Wait for FRAME_COUNT to change (syncs to 60 FPS)
; =============================================================================
WAIT_FRAME:
    WAIT_VSYNC LAST_FRAME_HI, LAST_FRAME_LO
    RET

; =============================================================================
; DATA SECTION - All graphics data defined here using ORG for positioning
; =============================================================================

section .data

; ============================================================================
; User Data (0x4000 - 0x408F)
; ============================================================================

ORG USER_DATA_BASE

; State flags
initialized:  DB 0                ; initialization flag (0xAA when done)
last_frame:   DB 0                ; last known frame count

; Sprite data array (32 sprites × 4 bytes = 128 bytes)
; Each sprite: [x, y, vel_x, vel_y]
ORG SPRITE_DATA_ADDR
sprite_data:  TIMES SPRITE_DATA_SIZE DB 0

; ============================================================================
; Tile 0: Smiley Face (32 bytes)
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
ORG TILE_DATA_ADDR
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
; OAM (64 entries × 4 bytes = 256 bytes)
; Format: [Y, X, tile, attr] - Y=0xFF means hidden
; All 64 sprites start hidden (Y = 0xFF)
; ============================================================================
ORG OAM_ADDR
oam_data:
    ; Each OAM entry: Y=0xFF (hidden), X=0, tile=0, attr=0
    TIMES 64 DB 0xFF, 0x00, 0x00, 0x00

; ============================================================================
; Palette (16 colors × 2 bytes = 32 bytes, using first 4)
; Format: [color_RGB332, padding]
; ============================================================================
ORG PALETTE_ADDR
palette:
    ; Color 0: dark blue background (RGB332: 0x03)
    DB 0x03, 0x00
    ; Color 1: blue border (RGB332: 0x1F)
    DB 0x1F, 0x00
    ; Color 2: yellow face (RGB332: 0xFC)
    DB 0xFC, 0x00
    ; Color 3: black eyes/mouth (RGB332: 0x00)
    DB 0x00, 0x00
