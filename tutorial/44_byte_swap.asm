; Exercise 44: Byte Swap (Nibble Swap)
; Level: 6 - Bit Manipulation
;
; Goal: Swap high and low nibbles (4-bit halves) of a byte
;       Value: 0xAB (1010 1011) → 0xBA (1011 1010)
;
; Note: Tiny16 immediates can only be used with LOADI (and with LOAD/STORE
;       address operands). For masks like 0x0F, LOADI them into a register
;       first, then AND/OR with that register.
;
; Instructions to use: LOADI, MOV, SHL, SHR, AND, OR, HALT
; Expected result: R0 = 0xBA
;
; Hint: Extract high nibble: (value >> 4) & 0x0F
; Hint: Extract low nibble: value & 0x0F
; Hint: Swap: (low << 4) | high
; Hint: Nibble = 4 bits, byte = 8 bits = 2 nibbles

section .code

; TODO: LOADI 0xAB into R0 (1010 1011)
; TODO: MOV R0 to R1 (save original)
; TODO: LOADI 0x0F into R2 (nibble mask)
; TODO: AND R0 with R2 (extract low nibble: 0x0B)
; TODO: Shift R0 left 4 times (move low nibble to high: 0xB0)
; TODO: MOV R1 to R3 (get original value)
; TODO: Shift R3 right 4 times (move high nibble to low: 0x0A)
; TODO: AND R3 with R2 (mask to ensure only low nibble: 0x0A)
; TODO: OR R0 with R3 (combine: 0xB0 | 0x0A = 0xBA)
; TODO: Halt (R0 should be 0xBA)
