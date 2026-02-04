; Exercise 39: Set and Clear Bits
; Level: 6 - Bit Manipulation
; Goal: Set specific bits to 1 using OR, clear specific bits to 0 using AND
;       Start with 0x00, set bits 0 and 4, then clear bit 0
; Instructions to use: LOADI, OR, AND, HALT
; Expected result: R0 = 0x10 (only bit 4 set)
; Hint: OR with 1-bits sets those bits: value | mask
; Hint: AND with 0-bits clears those bits: value & ~mask
; Hint: To clear bit N: AND with value that has bit N = 0, all others = 1

section .code

loadi   r0, 0b00000000
loadi   r1, 0b00010001
or      r0, r1
;           0b00010001
loadi   r1, 0b11111110
and     r0, r1
;           0b00010000
halt
