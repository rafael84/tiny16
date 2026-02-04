; Exercise 44: Byte Swap (Nibble Swap)
; Level: 6 - Bit Manipulation
; Goal: Swap high and low nibbles (4-bit halves) of a byte
;       Value: 0xAB (1010 1011) → 0xBA (1011 1010)
; Note: Tiny16 immediates can only be used with LOADI (and with LOAD/STORE
;       address operands). For masks like 0x0F, LOADI them into a register
;       first, then AND/OR with that register.
; Instructions to use: LOADI, MOV, SHL, SHR, AND, OR, HALT
; Expected result: R0 = 0xBA
; Hint: Extract high nibble: (value >> 4) & 0x0F
; Hint: Extract low nibble: value & 0x0F
; Hint: Swap: (low << 4) | high
; Hint: Nibble = 4 bits, byte = 8 bits = 2 nibbles

section .code

loadi   r0, 0b10101011
mov     r1, r0
loadi   r2, 0x0F

and     r0, r2
times 4 shl r0
mov     r3, r1
times 4 shr r3

and     r3, r2
or      r0, r3

halt
