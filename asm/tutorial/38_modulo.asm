; Exercise 38: Modulo Operation
; Level: 5 - Advanced Arithmetic
; Goal: Compute 17 % 5 = 2 (remainder after division)
;       Keep subtracting divisor until remainder < divisor
; Instructions to use: LOADI, SUB, CMP, JC, HALT
; Expected result: R0 = 2 (remainder)
; Hint: Modulo returns remainder: 17 = 3*5 + 2, so 17%5 = 2
; Hint: Similar to division but we keep the remainder, not the count
; Hint: Stop when dividend < divisor, remainder is what's left

section .code

loadi   r0, 17
loadi   r1, 5
loop:
    cmp     r0, r1
    jc      done
    sub     r0, r1
    jmp     loop
done:
    halt
