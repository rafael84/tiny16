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

; TODO: Load any non-zero value into R0 (e.g., 99)
; TODO: XOR R0 with itself to zero it
; TODO: Halt the program
