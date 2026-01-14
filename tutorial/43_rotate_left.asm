; Exercise 43: Rotate Left
; Level: 6 - Bit Manipulation
;
; Goal: Rotate bits left (circular shift): bit 7 wraps to bit 0
;       Value: 0x81 (10000001) rotate left → 0x03 (00000011)
;
; Instructions to use: LOADI, SHL, ADD, AND, OR, HALT
; Expected result: R0 = 0x03
;
; Hint: Rotate left = shift left + wrap bit 7 to bit 0
; Hint: Algorithm: save bit 7, shift left, if bit 7 was set, set bit 0
; Hint: Test bit 7 before shift using AND with 0x80
; Hint: Rotate differs from shift: bits don't fall off, they wrap

section .code

; TODO: Load 0x81 into R0 (10000001)
; TODO: Copy R0 to R1 (save for bit 7 test)
; TODO: AND R1 with 0x80 (test bit 7)
; TODO: SHL R0 (shift left: 10000001 → 00000010)
; TODO: If bit 7 was set (R1 != 0), OR R0 with 0x01 (set bit 0)
; TODO: Halt (R0 should be 0x03 = 00000011)
