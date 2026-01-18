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

loadi   r0, 17      ; Load 17 into R0 (dividend/remainder)
loadi   r1, 5       ; Load 5 into R1 (divisor)
loop:
    cmp     r0, r1  ; Compare R0 with R1 (check if R0 < R1)
    jc      done    ; If R0 < R1 (C=1), jump to done (R0 is the remainder)
    sub     r0, r1  ; Subtract R1 from R0
    jmp     loop    ; Jump back to loop
done:
    halt            ; Halt (R0 should be 2)
