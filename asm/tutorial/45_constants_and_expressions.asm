; Tutorial 45: Constants and Expressions
; Level: 7 - Assembler Power Features
; Goal: Use constants and expressions to compute immediates
; Expected result: R0 = 24, R1 = 0x13
; Hint: You can define constants with NAME = expression
; Hint: Expressions support + - * / % << >> & ^ | and parentheses

VALUE = (6 + 2) * 3
MASK  = (1 << 4) | 0x03

section .code

loadi r0, VALUE
loadi r1, MASK
halt
