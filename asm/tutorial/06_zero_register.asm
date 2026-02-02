; Exercise 06: Zero a Register
; Level: 1 - Fundamentals
;
; Goal: Load any value into R0, then set it to 0 using XOR (not using LOADI R0, 0)
;
; Instructions to use: LOADI, XOR, HALT
; Expected result: R0 = 0
;
; Hint: XOR R, R always equals 0 (any value XOR itself = 0)
; Hint: This is a common assembly idiom to zero a register
; Hint: XOR sets Z=1 (because result is 0) and clears C flag

section .code

loadi r0, 99    ; Load any non-zero value into R0 (e.g., 99)
xor r0, r0      ; XOR R0 with itself to zero it
halt            ; Halt the program
