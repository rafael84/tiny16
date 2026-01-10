; Exercise 1: Double a Number
;
; Goal: Load 5 into R0, then double it (result should be 10)
; Instructions to use: LOADI, ADD
; Expected result: R0 = 10

section .code

loadi r0, 5
loadi r1, 5
add r0, r1

halt
