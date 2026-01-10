; Exercise 6: Count to 10
;
; Goal: Count from 0 to 10 using a loop
; Instructions to use: LOADI, INC, SUB, JNZ
; Expected result: R0 = 10

section .code

loadi r0, 0
loadi r1, 10
loop:   inc r0
        dec r1
        jnz loop
halt

