START:
    ; Read timer for color
    LOADI R6, 0xBF
    LOADI R7, 0x20
    LOAD  R0

    ; Fill screen
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

    ; Signal frame complete - request swap
    LOADI R6, 0xBF
    LOADI R7, 0x23
    LOADI R0, 1
    STORE R0

    ; Wait for next frame
WAIT:
    LOADI R6, 0xBF
    LOADI R7, 0x22  ; FRAME_COUNT
    LOAD  R1
    SUB   R1, R2
    JZ    WAIT
    MOV   R2, R1

    JMP   START
