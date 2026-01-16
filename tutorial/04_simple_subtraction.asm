; Exercise 04: Simple Subtraction
; Level: 1 - Fundamentals
;
; Goal: Load 15 into R0, load 10 into R1, then subtract (R0 = R0 - R1 = 5)
;
; Instructions to use: LOADI, SUB, HALT
; Expected result: R0 = 5, R1 = 10
;
; Hint: SUB R1, R2 means R1 = R1 - R2 (result goes in first register)
; Hint: SUB sets C flag if borrow occurs (R1 < R2)

section .code

loadi r0, 15    ; Load 15 into R0
loadi r1, 10    ; Load 10 into R1
sub r0, r1      ; Subtract R1 from R0 (R0 = R0 - R1)
halt            ; Halt the program
