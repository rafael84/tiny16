; Exercise 26: Preserve Registers
; Level: 4 - Stack & Subroutines
;
; Goal: Save register values before modifying them, then restore
;       Simulate a function that uses R1 temporarily without clobbering it
;
; Instructions to use: LOADI, PUSH, POP, ADD, HALT
; Expected result: R0=15, R1=100 (R1 preserved despite being used)
;
; Hint: Pattern: PUSH before modifying, POP to restore
; Hint: This is essential for writing reusable functions
; Hint: Caller expects R1 to remain unchanged

section .code

loadi   r0, 5       ; Load 5 into R0
loadi   r1, 100     ; Load 100 into R1 (important value we need to preserve)
push    r1          ; Push R1 (save it before using R1 for temp work)
loadi   r1, 10      ; Load 10 into R1 (use R1 as temp)
add     r0, r1      ; Add R1 to R0 (R0 = 5 + 10 = 15)
pop     r1          ; Pop R1 (restore original value of 100)
halt                ; Halt (R0=15, R1=100)
