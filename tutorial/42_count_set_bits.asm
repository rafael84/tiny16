; Exercise 42: Count Set Bits (Population Count)
; Level: 6 - Bit Manipulation
;
; Goal: Count how many bits are set to 1 in a value
;       Value: 0x57 (01010111) has 5 bits set
;
; Instructions to use: LOADI, AND, JZ, INC, SHR, JMP, HALT
; Expected result: R0 = 5 (number of 1-bits in 0x57)
;
; Hint: Algorithm: test lowest bit, if 1 increment count, shift right, repeat
; Hint: Continue until all bits shifted out (value becomes 0)
; Hint: 0x57 = 01010111 has bits 0,1,2,4,6 set = 5 bits

section .code

; TODO: Load 0x57 into R1 (value to count)
; TODO: Load 0 into R0 (bit counter)
loop:
    ; TODO: Check if R1 == 0 (done when no bits left)
    ; TODO: If R1 == 0, jump to done
    ; TODO: Test lowest bit: AND R1 with 1, store in R2
    ; TODO: If bit is 0 (Z=1), skip increment
    ; TODO: If bit is 1, increment R0
    ; TODO: Shift R1 right (test next bit)
    ; TODO: Jump back to loop
done:
    ; TODO: Halt (R0 should be 5)
