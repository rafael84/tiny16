; Mouse Paint - Draw on screen with mouse
;
; Controls:
; - Left click: Draw white pixel at mouse position
; - Space key: Clear screen
;
; This demo shows:
; - Reading mouse position (MMIO 0xBF02, 0xBF03)
; - Reading mouse buttons (MMIO 0xBF04)
; - Reading keyboard input (MMIO 0xBF01)
; - Drawing pixels to framebuffer at mouse coordinates

section .code

START:
    ; Initialize if needed
    LOADI R6, 0x20
    LOADI R7, 0x00
    LOAD  R0          ; Check init flag
    LOADI R1, 0xAA
    SUB   R0, R1
    JZ    SKIP_INIT

    ; Clear screen on first run
    CALL  CLEAR_SCREEN

    ; Mark as initialized
    LOADI R0, 0xAA
    LOADI R6, 0x20
    LOADI R7, 0x00
    STORE R0

SKIP_INIT:
    CALL  READ_MOUSE
    CALL  WAIT_FRAME
    JMP   SKIP_INIT

; ============================================================================
; CLEAR_SCREEN - Clear framebuffer to black
; ============================================================================
CLEAR_SCREEN:
    LOADI R0, 0x00    ; Black
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

; ============================================================================
; READ_MOUSE - Read mouse input and handle buttons
; Modifies: R0-R7
; ============================================================================
READ_MOUSE:
    ; Read mouse position
    LOADI R6, 0xBF
    LOADI R7, 0x02
    LOAD  R0          ; R0 = mouse X (0-127)
    LOADI R7, 0x03
    LOAD  R1          ; R1 = mouse Y (0-127)

    ; Save position for later
    LOADI R6, 0x20
    LOADI R7, 0x02
    STORE R0          ; mouse_x
    LOADI R7, 0x03
    STORE R1          ; mouse_y

    ; Read mouse buttons
    LOADI R6, 0xBF
    LOADI R7, 0x04
    LOAD  R2          ; R2 = mouse buttons

    ; Check left button (bit 0) - draw pixel
    MOV   R3, R2
    LOADI R4, 0x01
    AND   R3, R4
    JZ    CHECK_KEYBOARD

    ; Draw pixel at mouse position
    CALL  DRAW_PIXEL

CHECK_KEYBOARD:
    ; Check Space key (bit 0) - clear screen
    LOADI R6, 0xBF
    LOADI R7, 0x01
    LOAD  R3          ; R3 = KEYS_PRESSED (auto-clears on read)
    LOADI R4, 0x01    ; bit 0 = Space
    AND   R3, R4
    JZ    DONE_MOUSE

    ; Clear screen
    CALL  CLEAR_SCREEN

DONE_MOUSE:
    RET

; ============================================================================
; DRAW_PIXEL - Draw white pixel at saved mouse position
; ============================================================================
DRAW_PIXEL:
    ; Load mouse position
    LOADI R6, 0x20
    LOADI R7, 0x02
    LOAD  R1          ; R1 = mouse_x
    LOADI R7, 0x03
    LOAD  R2          ; R2 = mouse_y

    ; Calculate framebuffer address: 0xC000 + (Y * 128) + X
    ; Framebuffer offset = (Y << 7) + X

    ; Calculate low byte and detect carry
    MOV   R6, R2
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6          ; R6 = (Y << 7) & 0xFF
    ADD   R6, R1      ; R6 = low byte, sets C if overflow
    MOV   R7, R6      ; R7 = low byte of address

    ; Calculate high byte: 0xC0 + (Y >> 1) + carry
    MOV   R6, R2
    SHR   R6          ; R6 = Y >> 1
    LOADI R5, 0xC0
    ADD   R6, R5      ; R6 = 0xC0 + (Y >> 1)

    ; Add carry from low byte addition if it occurred
    JNC   NO_CARRY
    INC   R6          ; Add carry to high byte

NO_CARRY:
    ; Draw white pixel
    LOADI R0, 0xFF
    STORE R0
    RET

; ============================================================================
; WAIT_FRAME - Signal frame complete
; ============================================================================
WAIT_FRAME:
    LOADI R6, 0xBF
    LOADI R7, 0x23
    LOADI R0, 1
    STORE R0
    RET

section .data

initialized: DB 0     ; Initialization flag
mouse_x:     DB 0     ; Mouse X position
mouse_y:     DB 0     ; Mouse Y position
