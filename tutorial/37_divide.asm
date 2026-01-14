; Exercise 37: 8-bit Division
; Level: 5 - Advanced Arithmetic
;
; Goal: Divide 42 / 7 = 6 using repeated subtraction
;       Count how many times divisor can be subtracted from dividend
;
; Instructions to use: LOADI, SUB, INC, CMP, JC, HALT
; Expected result: R0 = 6 (quotient)
;
; Hint: Division is repeated subtraction: 42-7-7-7-7-7-7 = 0 (6 times)
; Hint: Count subtractions until dividend < divisor
; Hint: Use CMP and JC to detect when dividend < divisor

section .code

; TODO: Load 42 into R1 (dividend)
; TODO: Load 7 into R2 (divisor)
; TODO: Load 0 into R0 (quotient counter)
loop:
    ; TODO: Compare R1 with R2 (check if R1 < R2)
    ; TODO: If R1 < R2 (C=1), jump to done
    ; TODO: Subtract R2 from R1
    ; TODO: Increment R0 (count subtractions)
    ; TODO: Jump back to loop
done:
    ; TODO: Halt (R0=6 quotient, R1=0 remainder)
