; Exercise 34: 16-bit Subtraction
; Level: 5 - Advanced Arithmetic
; Goal: Subtract two 16-bit numbers using SBC (subtract with carry/borrow)
;       0x0201 - 0x00FF = 0x0102 (513 - 255 = 258)
; Instructions to use: LOADI, SUB, SBC, HALT
; Expected result: R6:R7 = 0x0102 (R6 = 0x01, R7 = 0x02)
; Hint: SBC R1, R2 means R1 = R1 - R2 - C (subtract with borrow)
; Hint: First subtract low bytes (sets C if borrow), then SBC high bytes
; Hint: C flag = 1 means borrow occurred

section .code

; First number: 0x0201 (R6:R7)
loadi   r6, 0x02
loadi   r7, 0x01

; Second number: 0x00FF (R0:R1)
loadi   r0, 0x00
loadi   r1, 0xFF

; Subtract low bytes first
sub     r7, r1

; Subtract high bytes with borrow
sbc     r6, r0

; Result: R6:R7 = 0x0102
halt
