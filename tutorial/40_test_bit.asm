; Exercise 40: Test if Bit is Set
; Level: 6 - Bit Manipulation
;
; Goal: Test if bit 5 is set in a value (0x2A = 00101010)
;       Use AND to isolate the bit, then check if result is zero
;
; Instructions to use: LOADI, AND, JZ, HALT
; Expected result: R0 = 1 (bit 5 is set)
;
; Hint: Test bit N: AND with (1 << N), if result != 0, bit was set
; Hint: 0x2A = 00101010, bit 5 = 0, bit 3 = 1, bit 1 = 1
; Hint: AND sets Z flag if result is 0

section .code

; TODO: Load 0x2A into R0 (00101010)
; TODO: Load 0x20 into R1 (mask for bit 5: 00100000)
; TODO: AND R0 with R1 (isolate bit 5)
; TODO: If result is 0 (Z=1), jump to bit_clear
; TODO: Load 1 into R0 (bit was set)
; TODO: Jump to done
bit_clear:
    ; TODO: Load 0 into R0 (bit was clear)
done:
    ; TODO: Halt (R0=1 since bit 5 is set in 0x2A)
