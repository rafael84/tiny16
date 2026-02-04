; Tutorial 25: Push and Pop
; Level: 4 - Stack & Subroutines
; Goal: Learn basic stack operations - push values onto stack and pop them back
;       Push R0, R1, R2, then pop them back in reverse order
; Instructions to use: LOADI, PUSH, POP, HALT
; Expected result: R3=30, R4=20, R5=10 (values popped in reverse order)
; Hint: PUSH R stores register value at SP, then decrements SP
; Hint: POP R increments SP, then loads value into register
; Hint: Stack is LIFO: Last In, First Out
; Hint: Stack grows down from 0xBEFF

section .code

loadi   r0, 10
loadi   r1, 20
loadi   r2, 30
push    r0
push    r1
push    r2
pop     r3
pop     r4
pop     r5
halt
