; ======================================
; tiny16 demo: animated plasma (SAFE)
; ======================================

START:
MAIN_LOOP:
    LOADI R1, 0              ; y = 0

Y_LOOP:
    LOADI R0, 0              ; x = 0

X_LOOP:
    ; -------------------------
    ; pixel = x ^ y ^ timer
    ; -------------------------
    XOR   R2, R2             ; R2 = 0
    ADD   R2, R0             ; R2 = x
    XOR   R2, R1             ; R2 = x ^ y

    ; timer low byte
    LOADI R6, 0xE2
    LOADI R7, 0x00
    LOAD  R3
    XOR   R2, R3

    ; -------------------------
    ; framebuffer address
    ; addr = 0xC000 + (y<<8) + x
    ; -------------------------
    LOADI R6, 0xC0
    ADD   R6, R1             ; high byte = 0xC0 + y
    LOADI R7, 0x00
    ADD   R7, R0             ; low byte = x

    STORE R2

    ; -------------------------
    ; x++
    ; -------------------------
    INC   R0
    LOADI R3, 128
    SUB   R3, R0
    JNZ   X_LOOP

    ; -------------------------
    ; y++
    ; -------------------------
    INC   R1
    LOADI R3, 128
    SUB   R3, R1
    JNZ   Y_LOOP

    JMP MAIN_LOOP
