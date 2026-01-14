; Exercise 39: Set and Clear Bits
; Level: 6 - Bit Manipulation
;
; Goal: Set specific bits to 1 using OR, clear specific bits to 0 using AND
;       Start with 0x00, set bits 0 and 4, then clear bit 0
;
; Instructions to use: LOADI, OR, AND, HALT
; Expected result: R0 = 0x10 (only bit 4 set)
;
; Hint: OR with 1-bits sets those bits: value | mask
; Hint: AND with 0-bits clears those bits: value & ~mask
; Hint: To clear bit N: AND with value that has bit N = 0, all others = 1

section .code

; TODO: Load 0x00 into R0 (start with all bits clear)
; TODO: Load 0x11 into R1 (mask for bits 0 and 4: 00010001)
; TODO: OR R0 with R1 (set bits 0 and 4: R0 = 0x11)
; TODO: Load 0xFE into R1 (mask to clear bit 0: 11111110)
; TODO: AND R0 with R1 (clear bit 0: R0 = 0x10)
; TODO: Halt (R0 should be 0x10)
