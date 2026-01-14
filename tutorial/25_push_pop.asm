; Tutorial 25: Push and Pop
; Level: 4 - Stack & Subroutines
;
; Goal: Learn basic stack operations - push values onto stack and pop them back
;       Push R0, R1, R2, then pop them back in reverse order
;
; Instructions to use: LOADI, PUSH, POP, HALT
; Expected result: R3=30, R4=20, R5=10 (values popped in reverse order)
;
; Hint: PUSH R stores register value at SP, then decrements SP
; Hint: POP R increments SP, then loads value into register
; Hint: Stack is LIFO: Last In, First Out
; Hint: Stack grows down from 0xBEFF

section .code

; TODO: Load 10 into R0
; TODO: Load 20 into R1
; TODO: Load 30 into R2
; TODO: Push R0 (10 goes on stack)
; TODO: Push R1 (20 goes on stack)
; TODO: Push R2 (30 goes on stack)
; TODO: Pop into R3 (gets 30 - last pushed)
; TODO: Pop into R4 (gets 20)
; TODO: Pop into R5 (gets 10 - first pushed)
; TODO: Halt (R3=30, R4=20, R5=10)
