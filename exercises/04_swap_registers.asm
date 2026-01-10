; Exercise 4: Swap Two Registers
;
; Goal: Swap values between R0 and R1 using R2 as temporary
; Initial: R0 = 10, R1 = 20
; Final: R0 = 20, R1 = 10
; Instructions to use: LOADI, MOV
; Expected result: R0 = 20, R1 = 10

section .code

loadi r0, 10
loadi r1, 20

mov r2, r0
mov r0, r1
mov r1, r2

halt

