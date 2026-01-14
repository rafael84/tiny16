; Exercise 44: Byte Swap (Nibble Swap)
; Level: 6 - Bit Manipulation
;
; Goal: Swap high and low nibbles (4-bit halves) of a byte
;       Value: 0xAB (1010 1011) → 0xBA (1011 1010)
;
; Instructions to use: LOADI, SHL, SHR, AND, OR, HALT
; Expected result: R0 = 0xBA
;
; Hint: Extract high nibble: (value >> 4) & 0x0F
; Hint: Extract low nibble: value & 0x0F
; Hint: Swap: (low << 4) | high
; Hint: Nibble = 4 bits, byte = 8 bits = 2 nibbles

section .code

; TODO: Load 0xAB into R0 (1010 1011)
; TODO: Copy R0 to R1 (save original)
; TODO: AND R0 with 0x0F (extract low nibble: 0x0B)
; TODO: Shift R0 left 4 times (move low nibble to high: 0xB0)
; TODO: Move R1 to R2 (get original value)
; TODO: Shift R2 right 4 times (move high nibble to low: 0x0A)
; TODO: AND R2 with 0x0F (mask to ensure only low nibble: 0x0A)
; TODO: OR R0 with R2 (combine: 0xB0 | 0x0A = 0xBA)
; TODO: Halt (R0 should be 0xBA)
