; Exercise 27: Simple Subroutine
; Level: 4 - Stack & Subroutines
;
; Goal: Create a subroutine that doubles R0 using CALL and RET
;       Main: R0 = 5, call double_it, result R0 = 10
;
; Instructions to use: LOADI, CALL, RET, ADD, HALT
; Expected result: R0 = 10
;
; Hint: CALL pushes return address (PC) and jumps to subroutine
; Hint: RET pops return address and jumps back
; Hint: Subroutine can modify registers passed to it
; Hint: This is the foundation of structured programming

section .code

; TODO: Load 5 into R0
; TODO: Call double_it subroutine
; TODO: Halt (R0 should be 10)

double_it:
    ; TODO: Add R0 to itself (R0 = R0 + R0)
    ; TODO: Return to caller
