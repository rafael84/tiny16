; Exercise 09: Unconditional Jump
; Level: 2 - Control Flow
;
; Goal: Create an infinite loop using JMP and a label
;       Load different values in each iteration to show it's looping
;
; Instructions to use: LOADI, JMP, INC, HALT
; Expected result: Infinite loop (will need -m flag to limit execution)
;
; Hint: Labels are defined with a colon: loop:
; Hint: JMP label jumps unconditionally to that label
; Hint: Use the emulator with -m 10 to limit to 10 instructions

section .code

loadi r0, 0     ; Load 0 into R0

loop:
    inc r0      ; Increment R0
    jmp loop    ; Jump back to loop
; Note: This program never reaches HALT - that's the point!
