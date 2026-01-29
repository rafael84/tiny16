; Mouse Paint - Draw on screen with mouse
;
; Controls:
; - Left click + drag: Draw white line
; - Right click + drag: Erase
; - Space key: Clear screen
;
; Uses line interpolation to handle fast mouse movements.
; Demonstrates expressions in constants for memory layout.

.include "../stdlib/tiny16.inc"

; =============================================================================
; Constants - Memory Map
; =============================================================================

; Data section base address
USER_DATA_BASE    = 0x4000

; Data layout offsets
INITIALIZED_ADDR  = USER_DATA_BASE + 0
CUR_X_ADDR        = USER_DATA_BASE + 1
CUR_Y_ADDR        = USER_DATA_BASE + 2
LAST_FRAME_ADDR   = USER_DATA_BASE + 3
PREV_X_ADDR       = USER_DATA_BASE + 4
PREV_Y_ADDR       = USER_DATA_BASE + 5
DRAW_COLOR_ADDR   = USER_DATA_BASE + 6
DRAW_X_ADDR       = USER_DATA_BASE + 16
DRAW_Y_ADDR       = USER_DATA_BASE + 17
SX_ADDR           = USER_DATA_BASE + 18
ABS_DX_ADDR       = USER_DATA_BASE + 19
SY_ADDR           = USER_DATA_BASE + 20
ABS_DY_ADDR       = USER_DATA_BASE + 21
STEPS_ADDR        = USER_DATA_BASE + 22


; FRAMEBUFFER_BASE, FRAMEBUFFER_HI, FRAMEBUFFER_LO provided by stdlib


; =============================================================================
; Constants - Input & Graphics
; =============================================================================

; Mouse button masks
MOUSE_LEFT        = 0x01
MOUSE_RIGHT       = 0x02

; Keyboard masks
KEY_SPACE         = 0x01

; Drawing parameters
INITIALIZED_FLAG  = 0xAA
CENTER_POS        = 64
COLOR_WHITE       = 0xFF
COLOR_BG          = 0x03
SIGN_BIT_MASK     = 0x80
SIGN_POSITIVE     = 0x01
SIGN_NEGATIVE     = 0xFF

section .code

START:
    LOAD16 R0, INITIALIZED_ADDR
    CMPI R0, INITIALIZED_FLAG
    JZ    MAIN_LOOP

    CLEAR_SCREEN COLOR_BG

    ; Init prev position
    SETADDR PREV_X_ADDR
    LOADI R0, CENTER_POS
    STORE R0, [R6:R7]
    INC   R7
    STORE R0, [R6:R7]

    ; Mark initialized
    STORE16I INITIALIZED_FLAG, INITIALIZED_ADDR

MAIN_LOOP:
    CALL  WAIT_FRAME
    CALL  READ_MOUSE
    JMP   MAIN_LOOP

; =============================================================================
READ_MOUSE:
    ; Read mouse position
    READ_MOUSE_X R0
    READ_MOUSE_Y R1

    ; Save current position
    SETADDR CUR_X_ADDR
    STORE R0, [R6:R7]
    INC   R7
    STORE R1, [R6:R7]

    ; Read buttons
    READ_MOUSE_BUTTONS R2

    ; Left button - draw white
    SKIP_IF_CLEAR R2, MOUSE_LEFT, CHECK_RIGHT
    STORE16I COLOR_WHITE, DRAW_COLOR_ADDR
    CALL  DRAW_LINE
    JMP   UPDATE_PREV

CHECK_RIGHT:
    SKIP_IF_CLEAR R2, MOUSE_RIGHT, CHECK_SPACE
    STORE16I COLOR_BG, DRAW_COLOR_ADDR
    CALL  DRAW_LINE
    JMP   UPDATE_PREV

CHECK_SPACE:
    READ_KEYS_PRESSED R3
    SKIP_IF_CLEAR R3, KEY_SPACE, UPDATE_PREV
    CLEAR_SCREEN COLOR_BG

UPDATE_PREV:
    SETADDR CUR_X_ADDR
    LOAD  R0, [R6:R7]
    INC   R7
    LOAD  R1, [R6:R7]
    STORE16 R0, PREV_X_ADDR
    STORE16 R1, PREV_Y_ADDR
    RET

; =============================================================================
; DRAW_LINE - Simple stepping from prev to current
; =============================================================================
DRAW_LINE:
    ; Load prev (x0,y0) and cur (x1,y1)
    LOAD16 R0, PREV_X_ADDR     ; x0
    LOAD16 R1, PREV_Y_ADDR     ; y0
    LOAD16 R2, CUR_X_ADDR      ; x1
    LOAD16 R3, CUR_Y_ADDR      ; y1

    ; Store current draw pos
    STORE16 R0, DRAW_X_ADDR
    STORE16 R1, DRAW_Y_ADDR

    ; Calculate dx = x1 - x0, get sign and abs
    PUSH  R2
    PUSH  R3
    SUB   R2, R0      ; dx
    SUB   R3, R1      ; dy

    ; abs(dx) and sx
    MOV   R4, R2
    LOADI R5, SIGN_BIT_MASK
    AND   R4, R5
    JZ    DX_POS
    LOADI R4, 0
    SUB   R4, R2
    MOV   R2, R4
    LOADI R4, SIGN_NEGATIVE
    JMP   SAVE_SX
DX_POS:
    LOADI R4, SIGN_POSITIVE
SAVE_SX:
    STORE16 R4, SX_ADDR        ; sx
    STORE16 R2, ABS_DX_ADDR    ; abs_dx

    ; abs(dy) and sy
    MOV   R4, R3
    LOADI R5, SIGN_BIT_MASK
    AND   R4, R5
    JZ    DY_POS
    LOADI R4, 0
    SUB   R4, R3
    MOV   R3, R4
    LOADI R4, SIGN_NEGATIVE
    JMP   SAVE_SY
DY_POS:
    LOADI R4, SIGN_POSITIVE
SAVE_SY:
    STORE16 R4, SY_ADDR        ; sy
    STORE16 R3, ABS_DY_ADDR    ; abs_dy

    ; steps = max(abs_dx, abs_dy)
    CMP   R2, R3
    JNC   USE_DY
    MOV   R0, R2
    JMP   SET_STEPS
USE_DY:
    MOV   R0, R3
SET_STEPS:
    INC   R0          ; +1 for both endpoints
    STORE16 R0, STEPS_ADDR     ; steps

    POP   R3
    POP   R2

LINE_LOOP:
    CALL  DRAW_PIX

    LOAD16 R0, STEPS_ADDR
    DEC   R0
    STORE16 R0, STEPS_ADDR
    JZ    LINE_END

    ; Load draw position and abs values
    LOAD16 R0, DRAW_X_ADDR     ; x
    LOAD16 R1, DRAW_Y_ADDR     ; y
    LOAD16 R2, SX_ADDR         ; sx
    LOAD16 R3, ABS_DX_ADDR     ; abs_dx
    LOAD16 R4, SY_ADDR         ; sy
    LOAD16 R5, ABS_DY_ADDR     ; abs_dy

    ; Compare abs_dx vs abs_dy to decide stepping
    CMP   R3, R5
    JNC   STEP_Y_DOMINANT

    ; X-dominant: always step X, conditionally step Y
    LOADI R6, SIGN_POSITIVE
    CMP   R2, R6
    JNZ   STEP_X_NEG1
    INC   R0
    JMP   MAYBE_STEP_Y1
STEP_X_NEG1:
    DEC   R0
MAYBE_STEP_Y1:
    ; Step Y every (abs_dx/abs_dy) steps - simplified: check accumulator
    ; For simplicity, step Y when abs_dy > 0
    LOADI R6, 0
    CMP   R5, R6
    JZ    SAVE_LINE_POS
    LOADI R6, SIGN_POSITIVE
    CMP   R4, R6
    JNZ   STEP_Y_NEG1
    INC   R1
    JMP   SAVE_LINE_POS
STEP_Y_NEG1:
    DEC   R1
    JMP   SAVE_LINE_POS

STEP_Y_DOMINANT:
    ; Y-dominant: always step Y, conditionally step X
    LOADI R6, SIGN_POSITIVE
    CMP   R4, R6
    JNZ   STEP_Y_NEG2
    INC   R1
    JMP   MAYBE_STEP_X2
STEP_Y_NEG2:
    DEC   R1
MAYBE_STEP_X2:
    LOADI R6, 0
    CMP   R3, R6
    JZ    SAVE_LINE_POS
    LOADI R6, SIGN_POSITIVE
    CMP   R2, R6
    JNZ   STEP_X_NEG2
    INC   R0
    JMP   SAVE_LINE_POS
STEP_X_NEG2:
    DEC   R0

SAVE_LINE_POS:
    STORE16 R0, DRAW_X_ADDR
    STORE16 R1, DRAW_Y_ADDR
    JMP   LINE_LOOP

LINE_END:
    RET

; =============================================================================
DRAW_PIX:
    PUSH  R0
    PUSH  R1
    PUSH  R2
    PUSH  R4
    PUSH  R5
    PUSH  R6
    PUSH  R7

    LOAD16 R1, DRAW_X_ADDR     ; x
    LOAD16 R2, DRAW_Y_ADDR     ; y
    LOAD16 R5, DRAW_COLOR_ADDR ; color

    ; Address = 0xC000 + y*128 + x
    MOV   R6, R2
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    ADD   R6, R1
    MOV   R7, R6

    MOV   R6, R2
    SHR   R6
    LOADI R4, FRAMEBUFFER_HI
    ADD   R6, R4
    JNC   NO_C
    INC   R6
NO_C:
    STORE R5, [R6:R7]

    POP   R7
    POP   R6
    POP   R5
    POP   R4
    POP   R2
    POP   R1
    POP   R0
    RET

; =============================================================================
WAIT_FRAME:
    WAIT_VSYNC LAST_FRAME_ADDR
    RET

section .data

initialized:  DB 0     ; 0x4000
cur_x:        DB 0     ; 0x4001
cur_y:        DB 0     ; 0x4002
last_frame:   DB 0     ; 0x4003
prev_x:       DB 0     ; 0x4004
prev_y:       DB 0     ; 0x4005
draw_color:   DB 0     ; 0x4006
