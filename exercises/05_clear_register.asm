; Exercise 5: Clear a Register
;
; Goal: Set R0 to zero using XOR (without using LOADI R0, 0)
; Hint: Any number XOR itself equals 0
; Instructions to use: LOADI (to set initial value), XOR
; Expected result: R0 = 0

section .code

loadi r0, 0x44
xor r0, r0
halt
