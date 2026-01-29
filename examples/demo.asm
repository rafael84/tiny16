; Bouncing Sprites Demo - 32 PPU Sprites
;
; This demo demonstrates:
; - PPU tile-based rendering with tileset from PNG
; - Hardware sprites via OAM (32 bouncing sprites)
; - Using ORG to position data at specific memory addresses
; - Using expressions in constants for computed values
; - Animation synchronized to display frame rate
; - New addressing modes: post-increment [PAIR]+ and offset [PAIR + imm8]
;
; Memory layout (all defined in data section):
; USER_DATA_BASE+0:  initialized flag
; USER_DATA_BASE+1:  last_frame counter
; USER_DATA_BASE+16: sprite data (SPRITE_COUNT × SPRITE_SIZE bytes)
; TILE_DATA_ADDR:    Tiles from tileset.inc (128 tiles)
; OAM_ADDR:          OAM (64 entries × 4 bytes)
; PALETTE_ADDR:      Palette from tileset.inc
;

.include "../stdlib/tiny16.inc"

; =============================================================================
; Constants - Memory Map
; =============================================================================

; Data section base addresses
USER_DATA_BASE     = 0x4000
TILE_DATA_ADDR     = 0x5000
TILEMAP_ADDR       = 0x7000
OAM_ADDR           = 0x7800
PALETTE_ADDR       = 0x7900

; Data layout offsets
INITIALIZED_ADDR   = USER_DATA_BASE + 0
LAST_FRAME_ADDR    = USER_DATA_BASE + 1
SPRITE_DATA_ADDR   = USER_DATA_BASE + 16

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

TILE_SPRITE        = 68             ; Colorful gem sprite from tileset
TILE_BACKGROUND    = 0             ; Blue water/sky tile for background
SPRITE_ATTR_NONE   = 0x00

; PPU control flags
PPU_CTRL_BG_AND_SPRITES = 0x83      ; Enable BG (0x01) + Sprites (0x02) + Render (0x80)

section .code

START:
    ; Check if already initialized
    LOAD16 R0, INITIALIZED_ADDR
    LOADI R1, INITIALIZED_FLAG
    CMP   R0, R1
    JZ    MAIN_LOOP

    ; Initialize sprite positions (computed at runtime)
    CALL  INIT_SPRITES

    ; Initial render to start frame counter
    CALL  UPDATE_ALL_OAM
    CALL  RENDER_FRAME

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
    ; First, hide all 64 OAM sprites (set Y = 0xFF)
    SETADDR OAM_ADDR
    LOADI R0, 64              ; 64 sprites total
HIDE_SPRITES_LOOP:
    LOADI R1, 0xFF            ; Y = 0xFF (hidden)
    STORE R1, [R6:R7]+        ; Y
    LOADI R1, 0x00
    STORE R1, [R6:R7]+        ; X
    STORE R1, [R6:R7]+        ; tile
    STORE R1, [R6:R7]+        ; attr
    DEC   R0
    JNZ   HIDE_SPRITES_LOOP

    ; Fill tilemap with background tile (32x32 = 1024 entries, 2 bytes each)
    SETADDR TILEMAP_ADDR
    LOADI R4, 0               ; Counter high byte (need 1024 iterations)
    LOADI R5, 0               ; Counter low byte
FILL_TILEMAP_LOOP:
    LOADI R0, TILE_BACKGROUND
    STORE R0, [R6:R7]+        ; tile index
    LOADI R0, 0x00
    STORE R0, [R6:R7]+        ; attribute
    ; Increment 16-bit counter (R4:R5)
    INC   R5
    JNZ   FILL_TILEMAP_SKIP_HIGH
    INC   R4                  ; Carry to high byte
FILL_TILEMAP_SKIP_HIGH:
    ; Check if counter reached 1024 (0x0400) = R4=4, R5=0
    LOADI R0, 4
    CMP   R4, R0
    JNZ   FILL_TILEMAP_LOOP   ; R4 != 4, continue
    ; R4 == 4, check R5 == 0
    LOADI R0, 0
    CMP   R5, R0
    JNZ   FILL_TILEMAP_LOOP   ; R5 != 0, continue
    ; Done filling tilemap

    ; Read FRAME_COUNT for pseudo-random seed
    PUSH2 R6, R7
    READ_FRAME_COUNT R3            ; R3 = frame count (pseudo-random seed)
    POP2  R6, R7

    SETADDR SPRITE_DATA_ADDR
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
    STORE16I INITIALIZED_FLAG, INITIALIZED_ADDR

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
    LOADI R6, SPRITE_DATA_ADDR >> 8
    MOV   R7, R4
    MUL4  R7                   ; counter * 4
    LOADI R0, SPRITE_DATA_ADDR & 0xFF
    ADD   R7, R0

    ; Load x, y using offset addressing (no pointer modification needed)
    LOAD  R0, [R6:R7 + 0]      ; x at offset 0
    LOAD  R1, [R6:R7 + 1]      ; y at offset 1

    ; Calculate OAM address: 0x7800 + (counter * 4)
    LOADI R6, OAM_ADDR >> 8
    MOV   R7, R4
    MUL4  R7                   ; counter * 4
    LOADI R2, OAM_ADDR & 0xFF
    ADD   R7, R2

    ; Write OAM entry: Y, X, tile, attr
    OAM_WRITE_SPRITE R1, R0, TILE_SPRITE, SPRITE_ATTR_NONE

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
    LOADI R6, SPRITE_DATA_ADDR >> 8
    MOV   R7, R4
    MUL4  R7                   ; counter * 4
    LOADI R0, SPRITE_DATA_ADDR & 0xFF
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
    LOADI R6, SPRITE_DATA_ADDR >> 8
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
; RENDER_FRAME - Trigger PPU to render (background + sprites)
; =============================================================================
RENDER_FRAME:
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    LOADI R0, PPU_CTRL_BG_AND_SPRITES
    STORE R0, [R6:R7]
    RET

; =============================================================================
; WAIT_FRAME - Wait for FRAME_COUNT to change (syncs to 60 FPS)
; =============================================================================
WAIT_FRAME:
    WAIT_VSYNC LAST_FRAME_ADDR
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
; Tileset and Palette from tileset.inc
; - Tiles at 0x5000 (128 tiles × 64 bytes each)
; - Palette at 0x7900 (colors extracted from PNG)
; - OAM at 0x7800 is written at runtime (not pre-initialized)
; ============================================================================
.include "tileset.inc"
