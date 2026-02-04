; Exercise 35: 8-bit Multiplication (Simple)
; Level: 5 - Advanced Arithmetic
; Goal: Multiply 7 * 6 = 42 using repeated addition
;       result = a + a + a + ... (b times)
; Instructions to use: LOADI, ADD, DEC, JNZ, HALT
; Expected result: R0 = 42
; Hint: Multiplication is repeated addition: 7*6 = 7+7+7+7+7+7
; Hint: Use loop counter for number of additions
; Hint: Pattern: result = 0; loop b times: result += a

section .code

loadi   r1, 1
loadi   r2, 255
loadi   r0, 0
loop:
    add     r0, r1
    dec     r2
    jnz     loop
halt
