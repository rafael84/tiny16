; Input Test - Move a square with arrow keys
; Arrow keys or WASD to move
; Z/J to change color

section .code

START:
    ; Initialize position if not already done
    LOADI R6, 0x20
    LOADI R7, 0x10
    LOAD  R0          ; Check init flag
    LOADI R1, 0xAA
    SUB   R0, R1
    JZ    SKIP_INIT

    ; Set initial position (center)
    LOADI R0, 60
    LOADI R6, 0x20
    LOADI R7, 0x10
    STORE R0          ; pos_x = 60
    LOADI R7, 0x11
    STORE R0          ; pos_y = 60

    ; Set initial color
    LOADI R0, 0xFC    ; Yellow
    LOADI R7, 0x12
    STORE R0

    ; Mark as initialized
    LOADI R0, 0xAA
    LOADI R7, 0x13
    STORE R0

SKIP_INIT:
    CALL  CLEAR_SCREEN
    CALL  READ_INPUT
    CALL  DRAW_SQUARE
    CALL  WAIT_FRAME
    JMP   SKIP_INIT

; Clear screen to dark blue
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
        SUB   R1, R6
        JNZ   CLEAR_LOOP
        RET

; Read input and update position
READ_INPUT:
    ; Load current position
    LOADI R6, 0x20
    LOADI R7, 0x10
    LOAD  R1          ; R1 = pos_x
    LOADI R7, 0x11
    LOAD  R2          ; R2 = pos_y

    ; Read KEYS_STATE
    LOADI R6, 0xBF
    LOADI R7, 0x00
    LOAD  R0          ; R0 = keys

    ; Check Up (bit 6)
    MOV   R3, R0
    LOADI R4, 0x40
    AND   R3, R4
    JZ    CHECK_DOWN
    ; Move up
    MOV   R3, R2
    LOADI R4, 0
    SUB   R3, R4
    JZ    CHECK_DOWN
    DEC   R2

CHECK_DOWN:
    ; Check Down (bit 7)
    MOV   R3, R0
    LOADI R4, 0x80
    AND   R3, R4
    JZ    CHECK_LEFT
    ; Move down
    MOV   R3, R2
    LOADI R4, 119
    SUB   R3, R4
    JZ    CHECK_LEFT
    INC   R2

CHECK_LEFT:
    ; Check Left (bit 5)
    MOV   R3, R0
    LOADI R4, 0x20
    AND   R3, R4
    JZ    CHECK_RIGHT
    ; Move left
    MOV   R3, R1
    LOADI R4, 0
    SUB   R3, R4
    JZ    CHECK_RIGHT
    DEC   R1

CHECK_RIGHT:
    ; Check Right (bit 4)
    MOV   R3, R0
    LOADI R4, 0x10
    AND   R3, R4
    JZ    CHECK_BUTTON_A
    ; Move right
    MOV   R3, R1
    LOADI R4, 119
    SUB   R3, R4
    JZ    CHECK_BUTTON_A
    INC   R1

CHECK_BUTTON_A:
    ; Check A button (bit 2) - use KEYS_PRESSED for single press
    LOADI R6, 0xBF
    LOADI R7, 0x01
    LOAD  R3          ; R3 = KEYS_PRESSED (auto-clears)
    LOADI R4, 0x04
    AND   R3, R4
    JZ    SAVE_POS
    ; Change color
    LOADI R6, 0x20
    LOADI R7, 0x12
    LOAD  R3
    LOADI R4, 0x1C
    XOR   R3, R4      ; Toggle some color bits
    STORE R3

SAVE_POS:
    ; Save new position
    LOADI R6, 0x20
    LOADI R7, 0x10
    STORE R1
    LOADI R7, 0x11
    STORE R2
    RET

; Draw 8x8 square at current position
DRAW_SQUARE:
    ; Load position and color
    LOADI R6, 0x20
    LOADI R7, 0x10
    LOAD  R1          ; R1 = pos_x
    LOADI R7, 0x11
    LOAD  R2          ; R2 = pos_y
    LOADI R7, 0x12
    LOAD  R4          ; R4 = color

    LOADI R3, 8       ; 8 rows

DRAW_ROW:
    PUSH  R1
    PUSH  R3
    LOADI R3, 8       ; 8 pixels per row

DRAW_PIXEL:
    ; Calculate framebuffer address
    PUSH  R1
    PUSH  R2
    PUSH  R3
    PUSH  R4

    MOV   R5, R2
    SHR   R5          ; R5 = Y / 2
    MOV   R6, R2
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6          ; R6 = (Y << 7) & 0xFF
    ADD   R6, R1
    MOV   R7, R6
    LOADI R6, 0xC0
    ADD   R6, R5

    POP   R4
    STORE R4
    POP   R3
    POP   R2
    POP   R1

    INC   R1
    DEC   R3
    JNZ   DRAW_PIXEL

    POP   R3
    POP   R1
    INC   R2
    DEC   R3
    JNZ   DRAW_ROW
    RET

; Wait for frame
WAIT_FRAME:
    LOADI R6, 0xBF
    LOADI R7, 0x23
    LOADI R0, 1
    STORE R0
    RET

section .data

pos_x:        DB 0
pos_y:        DB 0
color:        DB 0
initialized:  DB 0
