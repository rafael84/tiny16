; Mouse Paint - Draw on screen with mouse
;
; Controls:
; - Left click + drag: Draw white line
; - Right click + drag: Erase
; - Space key: Clear screen
;
; Uses line interpolation to handle fast mouse movements.

section .code

START:
    LOADI R6, 0x40
    LOADI R7, 0x00
    LOAD  R0
    LOADI R1, 0xAA
    CMP   R0, R1
    JZ    MAIN_LOOP

    CALL  CLEAR_SCREEN

    ; Init prev position
    LOADI R6, 0x40
    LOADI R7, 0x04
    LOADI R0, 64
    STORE R0
    INC   R7
    STORE R0

    ; Mark initialized
    LOADI R6, 0x40
    LOADI R7, 0x00
    LOADI R0, 0xAA
    STORE R0

MAIN_LOOP:
    CALL  WAIT_FRAME
    CALL  READ_MOUSE
    JMP   MAIN_LOOP

; =============================================================================
CLEAR_SCREEN:
    LOADI R0, 0x03
    LOADI R6, 0xC0
    LOADI R7, 0x00
CLEAR_LOOP:
    STORE R0
    INC   R7
    JNZ   CLEAR_LOOP
    INC   R6
    LOADI R1, 0
    CMP   R6, R1
    JNZ   CLEAR_LOOP
    RET

; =============================================================================
READ_MOUSE:
    ; Read mouse position
    LOADI R6, 0xBF
    LOADI R7, 0x02
    LOAD  R0
    LOADI R7, 0x03
    LOAD  R1

    ; Save current position
    LOADI R6, 0x40
    LOADI R7, 0x01
    STORE R0
    INC   R7
    STORE R1

    ; Read buttons
    LOADI R6, 0xBF
    LOADI R7, 0x04
    LOAD  R2

    ; Left button - draw white
    MOV   R3, R2
    LOADI R4, 0x01
    AND   R3, R4
    JZ    CHECK_RIGHT
    LOADI R0, 0xFF
    LOADI R6, 0x40
    LOADI R7, 0x06
    STORE R0
    CALL  DRAW_LINE
    JMP   UPDATE_PREV

CHECK_RIGHT:
    MOV   R3, R2
    LOADI R4, 0x02
    AND   R3, R4
    JZ    CHECK_SPACE
    LOADI R0, 0x03
    LOADI R6, 0x40
    LOADI R7, 0x06
    STORE R0
    CALL  DRAW_LINE
    JMP   UPDATE_PREV

CHECK_SPACE:
    LOADI R6, 0xBF
    LOADI R7, 0x01
    LOAD  R3
    LOADI R4, 0x01
    AND   R3, R4
    JZ    UPDATE_PREV
    CALL  CLEAR_SCREEN

UPDATE_PREV:
    LOADI R6, 0x40
    LOADI R7, 0x01
    LOAD  R0
    INC   R7
    LOAD  R1
    INC   R7
    INC   R7
    STORE R0
    INC   R7
    STORE R1
    RET

; =============================================================================
; DRAW_LINE - Simple stepping from prev to current
; =============================================================================
DRAW_LINE:
    ; Load prev (x0,y0) and cur (x1,y1)
    LOADI R6, 0x40
    LOADI R7, 0x04
    LOAD  R0          ; x0
    INC   R7
    LOAD  R1          ; y0
    LOADI R7, 0x01
    LOAD  R2          ; x1
    INC   R7
    LOAD  R3          ; y1

    ; Store current draw pos
    LOADI R7, 0x10
    STORE R0          ; draw_x
    INC   R7
    STORE R1          ; draw_y

    ; Calculate dx = x1 - x0, get sign and abs
    PUSH  R2
    PUSH  R3
    SUB   R2, R0      ; dx
    SUB   R3, R1      ; dy

    ; abs(dx) and sx
    MOV   R4, R2
    LOADI R5, 0x80
    AND   R4, R5
    JZ    DX_POS
    LOADI R4, 0
    SUB   R4, R2
    MOV   R2, R4
    LOADI R4, 0xFF
    JMP   SAVE_SX
DX_POS:
    LOADI R4, 0x01
SAVE_SX:
    LOADI R6, 0x40
    LOADI R7, 0x12
    STORE R4          ; sx
    INC   R7
    STORE R2          ; abs_dx

    ; abs(dy) and sy
    MOV   R4, R3
    LOADI R5, 0x80
    AND   R4, R5
    JZ    DY_POS
    LOADI R4, 0
    SUB   R4, R3
    MOV   R3, R4
    LOADI R4, 0xFF
    JMP   SAVE_SY
DY_POS:
    LOADI R4, 0x01
SAVE_SY:
    LOADI R6, 0x40
    LOADI R7, 0x14
    STORE R4          ; sy
    INC   R7
    STORE R3          ; abs_dy

    ; steps = max(abs_dx, abs_dy)
    CMP   R2, R3
    JNC   USE_DY
    MOV   R0, R2
    JMP   SET_STEPS
USE_DY:
    MOV   R0, R3
SET_STEPS:
    INC   R0          ; +1 for both endpoints
    LOADI R6, 0x40
    LOADI R7, 0x16
    STORE R0          ; steps

    POP   R3
    POP   R2

LINE_LOOP:
    CALL  DRAW_PIX

    LOADI R6, 0x40
    LOADI R7, 0x16
    LOAD  R0
    DEC   R0
    STORE R0
    JZ    LINE_END

    ; Load draw position
    LOADI R7, 0x10
    LOAD  R0          ; x
    INC   R7
    LOAD  R1          ; y

    ; Load abs values
    INC   R7
    LOAD  R2          ; sx
    INC   R7
    LOAD  R3          ; abs_dx
    INC   R7
    LOAD  R4          ; sy
    INC   R7
    LOAD  R5          ; abs_dy

    ; Compare abs_dx vs abs_dy to decide stepping
    CMP   R3, R5
    JNC   STEP_Y_DOMINANT

    ; X-dominant: always step X, conditionally step Y
    LOADI R6, 1
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
    LOADI R6, 1
    CMP   R4, R6
    JNZ   STEP_Y_NEG1
    INC   R1
    JMP   SAVE_LINE_POS
STEP_Y_NEG1:
    DEC   R1
    JMP   SAVE_LINE_POS

STEP_Y_DOMINANT:
    ; Y-dominant: always step Y, conditionally step X
    LOADI R6, 1
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
    LOADI R6, 1
    CMP   R2, R6
    JNZ   STEP_X_NEG2
    INC   R0
    JMP   SAVE_LINE_POS
STEP_X_NEG2:
    DEC   R0

SAVE_LINE_POS:
    LOADI R6, 0x40
    LOADI R7, 0x10
    STORE R0
    INC   R7
    STORE R1
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

    LOADI R6, 0x40
    LOADI R7, 0x10
    LOAD  R1          ; x
    INC   R7
    LOAD  R2          ; y
    LOADI R7, 0x06
    LOAD  R5          ; color

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
    LOADI R4, 0xC0
    ADD   R6, R4
    JNC   NO_C
    INC   R6
NO_C:
    STORE R5

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
    LOADI R6, 0xBF
    LOADI R7, 0x22
    LOAD  R0
    LOADI R6, 0x40
    LOADI R7, 0x03
    LOAD  R1
    CMP   R0, R1
    JZ    WAIT_FRAME
    STORE R0
    RET

section .data

initialized:  DB 0     ; 0x4000
cur_x:        DB 0     ; 0x4001
cur_y:        DB 0     ; 0x4002
last_frame:   DB 0     ; 0x4003
prev_x:       DB 0     ; 0x4004
prev_y:       DB 0     ; 0x4005
draw_color:   DB 0     ; 0x4006
