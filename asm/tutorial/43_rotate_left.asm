; Exercise 43: Rotate Left
; Level: 6 - Bit Manipulation
;
; Goal: Rotate bits left (circular shift): bit 7 wraps to bit 0
;       Value: 0x81 (10000001) rotate left → 0x03 (00000011)
;
; Note: Tiny16 immediates can only be used with LOADI (and with LOAD/STORE
;       address operands). For bit masks like 0x80/0x01, LOADI them into a
;       register first, then AND/OR with that register.
;
; Instructions to use: LOADI, MOV, SHL, AND, OR, JNC, HALT
; Expected result: R0 = 0x03
;
; Hint: Rotate left = shift left + wrap bit 7 to bit 0
; Hint: Algorithm: save bit 7, shift left, if bit 7 was set, set bit 0
; Hint: Test bit 7 before shift using AND with 0x80
; Hint: Rotate differs from shift: bits don't fall off, they wrap

section .code

loadi   r0, 0b10000001  ; Load 0x81 into R0 (10000001)
mov     r1, r0          ; Copy R0 to R1 (save for bit 7 test)

loadi   r2, 0x80
and     r1, r2          ; AND R1 with 0x80 (test bit 7)

shl     r0              ; SHL R0 (shift left: 10000001 → 00000010) (1 goes into C)
jnc     done            ; If bit 7 was not set (C=0), skip wrapping bit 0

loadi   r3, 0x01        ; Load bit-0 mask (0x01) into R3
or      r0, r3          ; Set bit 0 (wrap): R0 |= 0x01

done:
    halt                ; Halt (R0 should be 0x03 = 00000011)
