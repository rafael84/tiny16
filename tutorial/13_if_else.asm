; Exercise 13: If-Else Logic
; Level: 2 - Control Flow
;
; Goal: Implement if-else logic
;       If R0 >= 10, set R2 to 100
;       Else, set R2 to 50
;
; Instructions to use: LOADI, CMP, JC, JMP, HALT
; Expected result: R2 = 50 (since R0 = 7 < 10)
;
; Hint: Structure is: compare, conditional jump to else, then-block, jump over else, else-block
; Hint: CMP followed by JC tests for "less than"

section .code

; TODO: Load 7 into R0
; TODO: Load 10 into R1 (threshold)
; TODO: Compare R0 with R1
; TODO: If R0 < R1 (C=1), jump to else_block
then_block:
    ; TODO: Load 100 into R2
    ; TODO: Jump to done
else_block:
    ; TODO: Load 50 into R2
done:
    ; TODO: Halt (R2 should be 50)
