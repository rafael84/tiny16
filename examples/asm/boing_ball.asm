.include "tiny16.inc"

; =============================================================================
; Boing Ball (Atari 2600-inspired) - Tiny16 framebuffer port
;
; Source inspiration:
; https://raw.githubusercontent.com/dasm-assembler/dasm/master/test/atari2600/boing26.asm
;
; - Renders to the 128x128 framebuffer at 0xC000.
; - Tiny16 MMIO has no audio, so "bounce" is a brief flash.
; - Toggle "egg mode" with Select (bit 0) to use the egg bitmap and color-cycle.
;
; Note: No @labels here (those are macro-local only).
; =============================================================================

; =============================================================================
; Constants
; =============================================================================

USER_DATA_BASE     = 0x4000

; FRAMEBUFFER_BASE, FRAMEBUFFER_HI, FRAMEBUFFER_LO provided by stdlib

INITIALIZED_FLAG   = 0xAA

BALL_W             = 40
BALL_H             = 50
MAX_X              = 128 - BALL_W   ; 88
MAX_Y              = 128 - BALL_H   ; 78

BG_COLOR           = 0x03           ; dark blue
BALL_COLOR         = 0xE0           ; red-ish (RGB332)
FLASH_COLOR        = 0xFF           ; white
FLASH_FRAMES       = 8

SKIP_ANIM_RESET    = 3              ; match boing26 "skip a few frames"

; KEY_SELECT and other key constants now provided by constants.inc

START_X            = MAX_X / 2
START_Y            = MAX_Y / 2
ROW_ADV            = 128 - BALL_W

FRAME_PTR_TABLE_HI = 0x40           ; 0x4000 + 15 bytes of state = 0x400F
FRAME_PTR_TABLE_LO = 0x11           ; 0x4000 + 17 bytes of state = 0x4011

; user-data addresses (ORG 0x4000)
INITIALIZED_ADDR   = USER_DATA_BASE + 0
LAST_FRAME_ADDR    = USER_DATA_BASE + 1
POS_X_ADDR         = USER_DATA_BASE + 2
POS_Y_ADDR         = USER_DATA_BASE + 3
PREV_X_ADDR        = USER_DATA_BASE + 4
PREV_Y_ADDR        = USER_DATA_BASE + 5
DIR_X_ADDR         = USER_DATA_BASE + 6
DIR_Y_ADDR         = USER_DATA_BASE + 7
SKIP_MOVE_ADDR     = USER_DATA_BASE + 8
SKIP_ANIM_ADDR     = USER_DATA_BASE + 9
ROT_DIR_ADDR       = USER_DATA_BASE + 10
ANIM_IDX_ADDR      = USER_DATA_BASE + 11
EGG_MODE_ADDR      = USER_DATA_BASE + 12
FRAME_CYCLE_ADDR   = USER_DATA_BASE + 13
FLASH_ADDR         = USER_DATA_BASE + 14
PREV_BM_HI_ADDR    = USER_DATA_BASE + 15
PREV_BM_LO_ADDR    = USER_DATA_BASE + 16

; Bitmaps are placed at ORG 0x4100, in this order:
; boing_egg then boing_frame_0..7, each 250 bytes.
;
; Precomputed addresses (DB-friendly, no expressions inside DB):
BOING_EGG_HI       = 0x41
BOING_EGG_LO       = 0x00

BOING_FRAME0_HI    = 0x41
BOING_FRAME0_LO    = 0xFA
BOING_FRAME1_HI    = 0x42
BOING_FRAME1_LO    = 0xF4
BOING_FRAME2_HI    = 0x43
BOING_FRAME2_LO    = 0xEE
BOING_FRAME3_HI    = 0x44
BOING_FRAME3_LO    = 0xE8
BOING_FRAME4_HI    = 0x45
BOING_FRAME4_LO    = 0xE2
BOING_FRAME5_HI    = 0x46
BOING_FRAME5_LO    = 0xDC
BOING_FRAME6_HI    = 0x47
BOING_FRAME6_LO    = 0xD6
BOING_FRAME7_HI    = 0x48
BOING_FRAME7_LO    = 0xD0

section .code

START:
    LOAD16 R0, INITIALIZED_ADDR
    LOADI R1, INITIALIZED_FLAG
    CMP   R0, R1
    JZ    MAIN_LOOP

    CLEAR_SCREEN BG_COLOR
    CALL  INIT_STATE
    STORE16I INITIALIZED_FLAG, INITIALIZED_ADDR

MAIN_LOOP:
    CALL  WAIT_FRAME
    CALL  HANDLE_INPUT
    CALL  UPDATE_STATE
    CALL  RENDER_FRAME
    JMP   MAIN_LOOP

INIT_STATE:
    ; Start centered-ish
    SETADDR POS_X_ADDR
    LOADI R0, START_X
    STORE R0, [R6:R7]+              ; pos_x
    LOADI R0, START_Y
    STORE R0, [R6:R7]+              ; pos_y
    LOADI R0, START_X
    STORE R0, [R6:R7]+              ; prev_x
    LOADI R0, START_Y
    STORE R0, [R6:R7]+              ; prev_y
    LOADI R0, 0x01
    STORE R0, [R6:R7]+              ; dir_x
    STORE R0, [R6:R7]+              ; dir_y
    CLEAR R0
    STORE R0, [R6:R7]+              ; skip_move
    LOADI R0, SKIP_ANIM_RESET
    STORE R0, [R6:R7]+              ; skip_anim
    LOADI R0, 0x01
    STORE R0, [R6:R7]+              ; rot_dir
    CLEAR R0
    STORE R0, [R6:R7]+              ; anim_idx
    CLEAR R0
    STORE R0, [R6:R7]+              ; egg_mode
    CLEAR R0
    STORE R0, [R6:R7]+              ; frame_cycle
    CLEAR R0
    STORE R0, [R6:R7]               ; flash

    ; Default previous bitmap: frame 0 (so the first clear is safe)
    STORE16I BOING_FRAME0_HI, PREV_BM_HI_ADDR
    STORE16I BOING_FRAME0_LO, PREV_BM_LO_ADDR
    RET

WAIT_FRAME:
    WAIT_VSYNC LAST_FRAME_ADDR
    RET

HANDLE_INPUT:
    READ_KEYS_PRESSED R0
    LOADI R1, KEY_SELECT
    AND   R0, R1
    JZ    HANDLE_INPUT_DONE

    LOAD16 R0, EGG_MODE_ADDR
    LOADI R1, 0x01
    XOR   R0, R1
    STORE16 R0, EGG_MODE_ADDR
HANDLE_INPUT_DONE:
    RET

UPDATE_STATE:
    ; frame_cycle++
    LOAD16 R0, FRAME_CYCLE_ADDR
    INC   R0
    STORE16 R0, FRAME_CYCLE_ADDR

    ; skip_anim--
    LOAD16 R0, SKIP_ANIM_ADDR
    DEC   R0
    STORE16 R0, SKIP_ANIM_ADDR
    JNZ   UPDATE_STATE_SKIP_ANIM_DONE

    ; reset skip_anim
    STORE16I SKIP_ANIM_RESET, SKIP_ANIM_ADDR

    ; if not in egg mode, advance animation frame by rot_dir
    LOAD16 R0, EGG_MODE_ADDR
    JNZ   UPDATE_STATE_SKIP_ANIM_DONE

    ; anim_idx = (anim_idx + rot_dir) & 7
    LOAD16 R1, ANIM_IDX_ADDR
    LOAD16 R2, ROT_DIR_ADDR
    ADD   R1, R2
    LOADI R2, 0x07
    AND   R1, R2
    STORE16 R1, ANIM_IDX_ADDR

UPDATE_STATE_SKIP_ANIM_DONE:
    ; Move every frame for smoother/faster animation (was: every other frame)
    LOAD16 R0, SKIP_MOVE_ADDR
    INC   R0
    STORE16 R0, SKIP_MOVE_ADDR

    ; update X
    LOAD16 R0, POS_X_ADDR
    LOAD16 R1, DIR_X_ADDR
    LOADI R2, 0x80
    MOV   R3, R1
    AND   R3, R2
    JNZ   UPDATE_STATE_X_LEFT

UPDATE_STATE_X_RIGHT:
    LOADI R2, MAX_X
    DEC   R2                        ; Compare with MAX_X - 1
    CMP   R0, R2
    JZ    UPDATE_STATE_X_BOUNCE_RIGHT
    INC   R0
    JMP   UPDATE_STATE_X_STORE
UPDATE_STATE_X_BOUNCE_RIGHT:
    LOAD16 R0, DIR_X_ADDR
    LOADI R1, 0xFE
    XOR   R0, R1
    STORE16 R0, DIR_X_ADDR
    CALL  ON_BOUNCE
    LOAD16 R0, POS_X_ADDR
    JMP   UPDATE_STATE_X_STORE

UPDATE_STATE_X_LEFT:
    CLEAR R2
    CMP   R0, R2
    JZ    UPDATE_STATE_X_BOUNCE_LEFT
    DEC   R0
    JMP   UPDATE_STATE_X_STORE
UPDATE_STATE_X_BOUNCE_LEFT:
    LOAD16 R0, DIR_X_ADDR
    LOADI R1, 0xFE
    XOR   R0, R1
    STORE16 R0, DIR_X_ADDR
    CALL  ON_BOUNCE
    LOAD16 R0, POS_X_ADDR

UPDATE_STATE_X_STORE:
    STORE16 R0, POS_X_ADDR

    ; update Y
    LOAD16 R0, POS_Y_ADDR
    LOAD16 R1, DIR_Y_ADDR
    LOADI R2, 0x80
    MOV   R3, R1
    AND   R3, R2
    JNZ   UPDATE_STATE_Y_UP

UPDATE_STATE_Y_DOWN:
    LOADI R2, MAX_Y
    DEC   R2                        ; Compare with MAX_Y - 1
    CMP   R0, R2
    JZ    UPDATE_STATE_Y_BOUNCE_DOWN
    INC   R0
    JMP   UPDATE_STATE_Y_STORE
UPDATE_STATE_Y_BOUNCE_DOWN:
    LOAD16 R0, DIR_Y_ADDR
    LOADI R1, 0xFE
    XOR   R0, R1
    STORE16 R0, DIR_Y_ADDR
    CALL  ON_BOUNCE
    LOAD16 R0, POS_Y_ADDR
    JMP   UPDATE_STATE_Y_STORE

UPDATE_STATE_Y_UP:
    CLEAR R2
    CMP   R0, R2
    JZ    UPDATE_STATE_Y_BOUNCE_UP
    DEC   R0
    JMP   UPDATE_STATE_Y_STORE
UPDATE_STATE_Y_BOUNCE_UP:
    LOAD16 R0, DIR_Y_ADDR
    LOADI R1, 0xFE
    XOR   R0, R1
    STORE16 R0, DIR_Y_ADDR
    CALL  ON_BOUNCE
    LOAD16 R0, POS_Y_ADDR

UPDATE_STATE_Y_STORE:
    STORE16 R0, POS_Y_ADDR

UPDATE_STATE_DONE:
    RET

ON_BOUNCE:
    ; flash = FLASH_FRAMES
    STORE16I FLASH_FRAMES, FLASH_ADDR

    ; rot_dir ^= 0xFE  (toggles 0x01 <-> 0xFF)
    LOAD16 R0, ROT_DIR_ADDR
    LOADI R1, 0xFE
    XOR   R0, R1
    STORE16 R0, ROT_DIR_ADDR
    RET

RENDER_FRAME:
    ; Check if position changed - skip clear/draw if stationary
    LOAD16 R0, POS_X_ADDR
    LOAD16 R1, POS_Y_ADDR
    LOAD16 R2, PREV_X_ADDR
    LOAD16 R3, PREV_Y_ADDR
    CMP   R0, R2
    JNZ   RENDER_FRAME_DO_UPDATE
    CMP   R1, R3
    JZ    RENDER_FRAME_DONE

RENDER_FRAME_DO_UPDATE:
    ; Clear previous sprite pixels (masked) using the previous bitmap pointer.
    MOV   R0, R2
    MOV   R1, R3
    LOAD16 R4, PREV_BM_HI_ADDR
    LOAD16 R5, PREV_BM_LO_ADDR
    LOADI R2, BG_COLOR
    CALL  DRAW_MASKED

    ; draw current
    LOAD16 R0, POS_X_ADDR
    LOAD16 R1, POS_Y_ADDR
    CALL  DRAW_BALL

    ; prev = pos
    LOAD16 R0, POS_X_ADDR
    LOAD16 R1, POS_Y_ADDR
    STORE16 R0, PREV_X_ADDR
    STORE16 R1, PREV_Y_ADDR

RENDER_FRAME_DONE:
    RET


; DRAW_BALL - draw 40x50 1bpp bitmap to framebuffer (masked: only 1-bits)
; Also updates prev bitmap pointer (for next frame's clear pass).
; Input: R0=x, R1=y
DRAW_BALL:
    CALL  SELECT_COLOR               ; -> R2
    CALL  SELECT_BITMAP_PTR          ; -> R4:R5

    ; save current bitmap pointer for next frame's clear pass
    STORE16 R4, PREV_BM_HI_ADDR
    STORE16 R5, PREV_BM_LO_ADDR

    CALL  DRAW_MASKED
    RET

; SELECT_COLOR - compute draw color in R2
; - if flash>0 => FLASH_COLOR and flash--
; - else if egg_mode => frame_cycle
; - else BALL_COLOR
SELECT_COLOR:
    LOAD16 R2, FLASH_ADDR
    JZ    SELECT_COLOR_NO_FLASH
    DEC   R2
    STORE16 R2, FLASH_ADDR
    LOADI R2, FLASH_COLOR
    RET
SELECT_COLOR_NO_FLASH:
    LOAD16 R3, EGG_MODE_ADDR
    JZ    SELECT_COLOR_NORMAL
    LOAD16 R2, FRAME_CYCLE_ADDR
    RET
SELECT_COLOR_NORMAL:
    LOADI R2, BALL_COLOR
    RET

; SELECT_BITMAP_PTR - compute bitmap pointer in R4:R5
SELECT_BITMAP_PTR:
    LOAD16 R3, EGG_MODE_ADDR
    JZ    SELECT_BITMAP_PTR_ANIM
    LOADI R4, BOING_EGG_HI
    LOADI R5, BOING_EGG_LO
    RET
SELECT_BITMAP_PTR_ANIM:
    LOAD16 R3, ANIM_IDX_ADDR        ; 0..7
    MUL2  R3                        ; *2
    LOADI R6, FRAME_PTR_TABLE_HI
    LOADI R7, FRAME_PTR_TABLE_LO
    ADD   R7, R3
    LOAD  R4, [R6:R7]+
    LOAD  R5, [R6:R7]
    RET

; Draw 8 pixels from the bits in R0 (masked), advancing FB pointer.
; Uses: R0 pattern, R2 color, R6:R7 framebuffer pointer.
; Optimized: removed wraparound checks (sprite width is 40px < 128px line width)
.macro DRAW_MASKED_BITS
    SHL   R0
    JNC   @bit1
    STORE R2, [R6:R7]
@bit1:
    INC   R7
    SHL   R0
    JNC   @bit2
    STORE R2, [R6:R7]
@bit2:
    INC   R7
    SHL   R0
    JNC   @bit3
    STORE R2, [R6:R7]
@bit3:
    INC   R7
    SHL   R0
    JNC   @bit4
    STORE R2, [R6:R7]
@bit4:
    INC   R7
    SHL   R0
    JNC   @bit5
    STORE R2, [R6:R7]
@bit5:
    INC   R7
    SHL   R0
    JNC   @bit6
    STORE R2, [R6:R7]
@bit6:
    INC   R7
    SHL   R0
    JNC   @bit7
    STORE R2, [R6:R7]
@bit7:
    INC   R7
    SHL   R0
    JNC   @bit8
    STORE R2, [R6:R7]
@bit8:
    INC   R7
.endmacro

; DRAW_MASKED - draw 40x50 1bpp bitmap to framebuffer (only 1-bits)
; Input:
; - R0=x, R1=y
; - R2=color
; - R4:R5 = bitmap pointer (consumed via [R4:R5]+)
DRAW_MASKED:
    PUSH2 R6, R7

    CALL  FB_PTR_FROM_XY            ; -> R6:R7

    LOADI R3, BALL_H
DRAW_MASKED_ROW:
    ; 5 bytes per row (40 pixels) - fully unrolled for speed
    LOAD  R0, [R4:R5]+
    DRAW_MASKED_BITS

    LOAD  R0, [R4:R5]+
    DRAW_MASKED_BITS

    LOAD  R0, [R4:R5]+
    DRAW_MASKED_BITS

    LOAD  R0, [R4:R5]+
    DRAW_MASKED_BITS

    LOAD  R0, [R4:R5]+
    DRAW_MASKED_BITS

    ; next row: + (128 - 40) = +88
    ; Since we just added 40 to R7, adding 88 more gives us next row
    ; Optimized: 88 = 0x58, so just add it
    LOADI R0, 88
    ADD   R7, R0
    JNC   DRAW_MASKED_NO_CARRY_ROW
    INC   R6
DRAW_MASKED_NO_CARRY_ROW:
    DEC   R3
    JNZ   DRAW_MASKED_ROW

    POP2  R6, R7
    RET

; FB_PTR_FROM_XY - compute framebuffer pointer for (x,y)
; Input: R0=x, R1=y
; Output: R6:R7 = 0xC000 + y*128 + x
FB_PTR_FROM_XY:
    FB_ADDR_FROM_XY
    RET

section .data

ORG USER_DATA_BASE
initialized:     DB 0
last_frame:      DB 0
pos_x:           DB 0
pos_y:           DB 0
prev_x:          DB 0
prev_y:          DB 0
dir_x:           DB 0
dir_y:           DB 0
skip_move:       DB 0
skip_anim:       DB 0
rot_dir:         DB 0
anim_idx:        DB 0
egg_mode:        DB 0
frame_cycle:     DB 0
flash:           DB 0
prev_bm_hi:      DB 0
prev_bm_lo:      DB 0

; Pointer table (hi, lo) for frames 0..7
frame_ptr_table:
    DB 0x41, 0xFA
    DB 0x42, 0xF4
    DB 0x43, 0xEE
    DB 0x44, 0xE8
    DB 0x45, 0xE2
    DB 0x46, 0xDC
    DB 0x47, 0xD6
    DB 0x48, 0xD0

; Bitmaps (1bpp, row-major: 50 rows * 5 bytes)
ORG 0x4100
.include "../includes/boing_ball_gfx.inc"

