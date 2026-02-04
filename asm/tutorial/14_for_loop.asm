; Exercise 14: For Loop
; Level: 2 - Control Flow
; Goal: Sum numbers from 1 to 5 using a for-loop pattern
;       R0 = accumulator (sum), R1 = counter, R2 = limit
; Instructions to use: LOADI, ADD, INC, CMP, JNZ, HALT
; Expected result: R0 = 15 (1+2+3+4+5)
; Hint: For-loop pattern: initialize counter, loop body, increment, test, repeat
; Hint: Sum should include: 1+2+3+4+5 = 15

section .code

loadi r0, 0
loadi r1, 1
loadi r2  6
loop:
    add r0, r1
    inc r1
    cmp r1, r2
    jnz loop
halt
