; Tutorial 02: Add Two Numbers
; Level: 1 - Fundamentals
;
; Goal: Load 7 into R0, load 8 into R1, then add them (result = 15 in R0)
;
; Instructions to use: LOADI, ADD, HALT
; Expected result: R0 = 15, R1 = 8
;
; Hint: ADD R1, R2 means R1 = R1 + R2 (result goes in first register)
; Hint: ADD sets the Z flag if result is 0, C flag if result overflows

section .code

loadi r0, 7     ; Load 7 into R0
loadi r1, 8     ; Load 8 into R1
add r0, r1      ; Add R1 to R0 (R0 = R0 + R1)
halt            ; Halt the program
