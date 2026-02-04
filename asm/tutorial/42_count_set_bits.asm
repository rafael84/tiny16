; Exercise 42: Count Set Bits (Population Count)
; Level: 6 - Bit Manipulation
; Goal: Count how many bits are set to 1 in a value
;       Value: 0x57 (01010111) has 5 bits set
; Instructions to use: LOADI, AND, JZ, INC, SHR, JMP, HALT
; Expected result: R0 = 5 (number of 1-bits in 0x57)
; Hint: Algorithm: test lowest bit, if 1 increment count, shift right, repeat
; Hint: Continue until all bits shifted out (value becomes 0)
; Hint: 0x57 = 01010111 has bits 0,1,2,4,6 set = 5 bits

section .code

loadi   r1, 0b01010111
loadi   r0, 0
loop:
    loadi   r2, 0
    cmp     r1, r2
    jz      done
    loadi   r2, 1
    and     r2, r1
    jz      next
    inc     r0
next:
    shr     r1
    jmp     loop
done:
    halt
