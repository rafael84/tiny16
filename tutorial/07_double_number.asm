; Exercise 07: Double a Number
; Level: 1 - Fundamentals
;
; Goal: Load 10 into R0, then double it to 20
;
; Instructions to use: LOADI, SHL (or ADD), HALT
; Expected result: R0 = 20
;
; Hint: You can double a number two ways:
;       1. SHL R0 (shift left = multiply by 2)
;       2. ADD R0, R0 (add to itself)
; Hint: SHL sets C flag if bit 7 was 1 before shift

section .code

loadi r0, 10    ; Load 10 into R0
shl r0          ; Double R0 using SHL or ADD
halt            ; Halt the program
