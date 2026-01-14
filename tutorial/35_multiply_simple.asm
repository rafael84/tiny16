; Exercise 35: 8-bit Multiplication (Simple)
; Level: 5 - Advanced Arithmetic
;
; Goal: Multiply 7 * 6 = 42 using repeated addition
;       result = a + a + a + ... (b times)
;
; Instructions to use: LOADI, ADD, DEC, JNZ, HALT
; Expected result: R0 = 42
;
; Hint: Multiplication is repeated addition: 7*6 = 7+7+7+7+7+7
; Hint: Use loop counter for number of additions
; Hint: Pattern: result = 0; loop b times: result += a

section .code

; TODO: Load 7 into R1 (multiplicand - number to add)
; TODO: Load 6 into R2 (multiplier - how many times)
; TODO: Load 0 into R0 (result accumulator)
loop:
    ; TODO: Add R1 to R0
    ; TODO: Decrement R2
    ; TODO: Jump to loop if R2 != 0
; TODO: Halt (R0 should be 42)
