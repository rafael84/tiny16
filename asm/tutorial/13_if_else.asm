; Exercise 13: If-Else Logic
; Level: 2 - Control Flow
; Goal: Implement if-else logic
;       If R0 >= 10, set R2 to 100
;       Else, set R2 to 50
; Instructions to use: LOADI, CMP, JC, JMP, HALT
; Expected result: R2 = 50 (since R0 = 7 < 10)
; Hint: Structure is: compare, conditional jump to else, then-block, jump over else, else-block
; Hint: CMP followed by JC tests for "less than"

section .code

loadi r0, 7
loadi r1, 10
cmp r0, r1
jc else_block
then_block:
    loadi r2, 100
    jmp done
else_block:
    loadi r2, 50
done:
    halt
