; Exercise 40: Test if Bit is Set
; Level: 6 - Bit Manipulation
; Goal: Test if bit 5 is set in a value (0x2A = 00101010)
;       Use AND to isolate the bit, then check if result is zero
; Instructions to use: LOADI, AND, JZ, HALT
; Expected result: R0 = 1 (bit 5 is set)
; Hint: Test bit N: AND with (1 << N), if result != 0, bit was set
; Hint: 0x2A = 00101010, bit 5 = 0, bit 3 = 1, bit 1 = 1
; Hint: AND sets Z flag if result is 0

section .code

loadi   r0, 0b00101010
loadi   r1, 0b00100000
and     r0, r1
;           0b00100000
jz      bit_clear
loadi   r0, 1
jmp     done
bit_clear:
    loadi   r0, 0
done:
    halt
