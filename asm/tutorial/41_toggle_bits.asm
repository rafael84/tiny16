; Exercise 41: Toggle Bits
; Level: 6 - Bit Manipulation
; Goal: Toggle (flip) specific bits using XOR
;       Start with 0xAA (10101010), toggle bits 0 and 7
; Instructions to use: LOADI, XOR, HALT
; Expected result: R0 = 0x2B (00101011)
; Hint: XOR with 1-bits toggles those bits: 0→1, 1→0
; Hint: XOR is used for flipping bits, encryption, checksums
; Hint: 0xAA = 10101010, toggle bits 0,7 → 00101011 = 0x2B

section .code

loadi   r0, 0b10101010
loadi   r1, 0b10000001
xor     r0, r1
;           0b00101011
halt
