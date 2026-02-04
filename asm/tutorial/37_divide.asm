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

loadi   r1, 42      ; Load 42 into R1 (dividend)
loadi   r2, 7       ; Load 7 into R2 (divisor)
loadi   r0, 0       ; Load 0 into R0 (quotient counter)
loop:
    cmp     r1, r2  ; Compare R1 with R2 (check if R1 < R2)
    jc      done    ; If R1 < R2 (C=1), jump to done
    sub     r1, r2  ; Subtract R2 from R1
    inc     r0      ; Increment R0 (count subtractions)
    jmp     loop    ; Jump back to loop
done:
    halt            ; Halt (R0=6 quotient, R1=0 remainder)
