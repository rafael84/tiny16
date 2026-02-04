; Exercise 26: Preserve Registers
; Level: 4 - Stack & Subroutines
; Goal: Save register values before modifying them, then restore
;       Simulate a function that uses R1 temporarily without clobbering it
; Instructions to use: LOADI, PUSH, POP, ADD, HALT
; Expected result: R0=15, R1=100 (R1 preserved despite being used)
; Hint: Pattern: PUSH before modifying, POP to restore
; Hint: This is essential for writing reusable functions
; Hint: Caller expects R1 to remain unchanged

section .code

loadi   r0, 5
loadi   r1, 100
push    r1
loadi   r1, 10
add     r0, r1
pop     r1
halt
