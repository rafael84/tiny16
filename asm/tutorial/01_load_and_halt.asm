; Tutorial 01: Load and Halt
; Level: 1 - Fundamentals
;
; Goal: Load the value 42 into register R0, then halt the program
;
; Instructions to use: LOADI, HALT
; Expected result: R0 = 42
;
; Hint: LOADI R, imm8 loads an 8-bit immediate value into a register
; Hint: HALT stops program execution

section .code

loadi r0, 42    ; Load 42 into R0
halt            ; Halt the program
