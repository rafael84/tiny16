; Exercise 15: While Loop
; Level: 2 - Control Flow
; Goal: Implement a while loop - keep doubling R0 until it reaches or exceeds 100
;       Start with R0 = 3, double it repeatedly
; Instructions to use: LOADI, SHL (or ADD), CMP, JNC, JMP, HALT
; Expected result: R0 = 192 (3, 6, 12, 24, 48, 96, 192)
; Hint: While-loop pattern: test condition first, then body, then loop back
; Hint: Continue while R0 < 100 (use CMP and JNC)

section .code

loadi r0, 3
loadi r1, 100
loop:
    cmp r0, r1
    jnc done
    shl r0
    jmp loop
done:
    halt
