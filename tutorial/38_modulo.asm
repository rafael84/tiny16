; Exercise 38: Modulo Operation
; Level: 5 - Advanced Arithmetic
;
; Goal: Compute 17 % 5 = 2 (remainder after division)
;       Keep subtracting divisor until remainder < divisor
;
; Instructions to use: LOADI, SUB, CMP, JC, HALT
; Expected result: R0 = 2 (remainder)
;
; Hint: Modulo returns remainder: 17 = 3*5 + 2, so 17%5 = 2
; Hint: Similar to division but we keep the remainder, not the count
; Hint: Stop when dividend < divisor, remainder is what's left

section .code

; TODO: Load 17 into R0 (dividend/remainder)
; TODO: Load 5 into R1 (divisor)
loop:
    ; TODO: Compare R0 with R1 (check if R0 < R1)
    ; TODO: If R0 < R1 (C=1), jump to done (R0 is the remainder)
    ; TODO: Subtract R1 from R0
    ; TODO: Jump back to loop
done:
    ; TODO: Halt (R0 should be 2)
