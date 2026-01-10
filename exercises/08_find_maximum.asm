; Exercise 8: Find Maximum
;
; Goal: Compare two numbers and keep the larger one in R0
; Test with: A = 25, B = 17 (result should be 25)
; Instructions to use: LOADI, SUB, JC, MOV
; Expected result: R0 = 25

section .code

loadi r0, 25    ; A
loadi r1, 17    ; B

mov r2, r0      ; temp
sub r2, r1      ; A - B; carry (C) set when borrow (A < B)
jc B
A:  halt
B:  mov r0, r1
    halt
