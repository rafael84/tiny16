; Exercise 37: 8-bit Division
; Level: 5 - Advanced Arithmetic
; Goal: Divide 42 / 7 = 6 using repeated subtraction
;       Count how many times divisor can be subtracted from dividend
; Instructions to use: LOADI, SUB, INC, CMP, JC, HALT
; Expected result: R0 = 6 (quotient)
; Hint: Division is repeated subtraction: 42-7-7-7-7-7-7 = 0 (6 times)
; Hint: Count subtractions until dividend < divisor
; Hint: Use CMP and JC to detect when dividend < divisor

section .code

loadi   r1, 42
loadi   r2, 7
loadi   r0, 0
loop:
    cmp     r1, r2
    jc      done
    sub     r1, r2
    inc     r0
    jmp     loop
done:
    halt
