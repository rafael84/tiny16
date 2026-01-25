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

loadi   r0, 0b10101011  ; LOADI 0xAB into R0 (1010 1011)
mov     r1, r0          ; MOV R0 to R1 (save original)
loadi   r2, 0x0F        ; LOADI 0x0F into R2 (nibble mask)

and     r0, r2          ; AND R0 with R2 (extract low nibble: 0x0B)
times 4 shl r0          ; Shift R0 left 4 times (move low nibble to high: 0xB0)
mov     r3, r1          ; MOV R1 to R3 (get original value)
times 4 shr r3          ; Shift R3 right 4 times (move high nibble to low: 0x0A)

and     r3, r2          ; AND R3 with R2 (mask to ensure only low nibble: 0x0A)
or      r0, r3          ; OR R0 with R3 (combine: 0xB0 | 0x0A = 0xBA)

halt                    ; Halt (R0 should be 0xBA)
