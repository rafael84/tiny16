; Exercise 7: Sum N Numbers
;
; Goal: Calculate 1 + 2 + 3 + 4 + 5 = 15
; Instructions to use: LOADI, ADD, INC, SUB, JZ, JMP
; Expected result: R0 = 15

section .code

loadi r0, 0
loadi r1, 1
loadi r2, 5
loop:   add r0, r1
        inc r1
        dec r2
        jnz loop
halt

