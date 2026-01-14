; Exercise 41: Toggle Bits
; Level: 6 - Bit Manipulation
;
; Goal: Toggle (flip) specific bits using XOR
;       Start with 0xAA (10101010), toggle bits 0 and 7
;
; Instructions to use: LOADI, XOR, HALT
; Expected result: R0 = 0x2B (00101011)
;
; Hint: XOR with 1-bits toggles those bits: 0→1, 1→0
; Hint: XOR is used for flipping bits, encryption, checksums
; Hint: 0xAA = 10101010, toggle bits 0,7 → 00101011 = 0x2B

section .code

; TODO: Load 0xAA into R0 (10101010)
; TODO: Load 0x81 into R1 (mask for bits 0 and 7: 10000001)
; TODO: XOR R0 with R1 (toggle bits 0 and 7)
; TODO: Halt (R0 should be 0x2B = 00101011)
