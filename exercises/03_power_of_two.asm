; Exercise 3: Compute 2^3 = 8
;
; Goal: Calculate 8 using only LOADI and SHL (shift left)
; Remember: SHL multiplies by 2
; Instructions to use: LOADI, SHL
; Expected result: R0 = 8

section .code

loadi r0, 2
shl r0
shl r0
halt
