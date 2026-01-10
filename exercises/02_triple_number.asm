; Exercise 2: Triple a Number
;
; Goal: Load 7 into R0, then triple it (result should be 21)
; Instructions to use: LOADI, ADD
; Expected result: R0 = 21

section .code

loadi r0, 7
loadi r1, 7
add r0, r1
add r0, r1
halt
