; Exercise 36: 8-bit Multiplication (Fast)
; Level: 5 - Advanced Arithmetic
;
; Goal: Multiply 12 * 5 = 60 using shift-and-add algorithm
;       More efficient than repeated addition for larger numbers
;
; Instructions to use: LOADI, ADD, SHR, AND, JZ, JMP, HALT
; Expected result: R0 = 60
;
; Hint: Shift-and-add algorithm:
;       For each bit in multiplier:
;         if bit is 1: add multiplicand to result
;         shift multiplicand left (double it)
;         shift multiplier right
; Hint: This is how hardware multipliers work
; Hint: Much faster than simple repeated addition

section .code

; TODO: Load 12 into R1 (multiplicand)
; TODO: Load 5 into R2 (multiplier)
; TODO: Load 0 into R0 (result)
loop:
    ; TODO: Check if R2 == 0 (done when multiplier is 0)
    ; TODO: If R2 == 0, jump to done
    ; TODO: Test if lowest bit of R2 is 1 (AND R2 with 1, check result)
    ; TODO: If bit is 1, add R1 to R0
    ; TODO: Shift R1 left (SHL - multiply by 2)
    ; TODO: Shift R2 right (SHR - divide by 2)
    ; TODO: Jump back to loop
done:
    ; TODO: Halt (R0 should be 60)
