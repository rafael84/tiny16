; ======================================
; tiny16: color cycle
; ======================================

START:
    ; Read timer
    LOADI R6, 0xBF
    LOADI R7, 0x20
    LOAD  R0            ; R0 = color from timer

    ; Fill screen with this color
    LOADI R6, 0xC0
    LOADI R7, 0x00

FILL:
    STORE R0
    INC   R7
    JNZ   FILL
    INC   R6
    LOADI R1, 0xFF
    SUB   R1, R6
    JNZ   FILL

    JMP   START
