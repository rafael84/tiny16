; ======================================
; tiny16 demo: animated plasma (SAFE)
; ======================================

START:
MAIN_LOOP:
    ; framebuffer pointer = 0xC000
    LOADI R4, 0xC0
    LOADI R5, 0x00

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
    LOADI R6, 0xE2           ; timer address
    LOADI R7, 0x00
    LOAD  R3

    ; restore framebuffer pointer into R6:R7
    XOR   R6, R6
    ADD   R6, R4
    XOR   R7, R7
    ADD   R7, R5
    XOR   R2, R3

    ; -------------------------
    ; framebuffer address (R6:R7 already points at next pixel)
    ; -------------------------
    STORE R2

    ; advance framebuffer pointer stored in R4:R5
    INC   R5
    JNZ   FB_NEXT
    INC   R4

FB_NEXT:

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
