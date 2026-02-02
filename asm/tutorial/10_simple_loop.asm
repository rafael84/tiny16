; Exercise 10: Simple Loop
; Level: 2 - Control Flow
;
; Goal: Count down from 10 to 0 using a loop
;       Use DEC and JNZ to repeat while R0 is not zero
;
; Instructions to use: LOADI, DEC, JNZ, HALT
; Expected result: R0 = 0 (after counting down from 10)
;
; Hint: DEC sets the Z flag when result is 0
; Hint: JNZ (Jump if Not Zero) jumps when Z=0
; Hint: This creates a countdown loop

section .code

loadi r0, 10    ; Load 10 into R0

loop:
    dec r0      ; Decrement R0
    jnz loop    ; Jump to loop if R0 is not zero (JNZ)
halt            ; Halt (R0 should be 0)
