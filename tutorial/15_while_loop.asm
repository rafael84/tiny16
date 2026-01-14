; Exercise 15: While Loop
; Level: 2 - Control Flow
;
; Goal: Implement a while loop - keep doubling R0 until it reaches or exceeds 100
;       Start with R0 = 3, double it repeatedly
;
; Instructions to use: LOADI, SHL (or ADD), CMP, JC, HALT
; Expected result: R0 = 192 (3, 6, 12, 24, 48, 96, 192)
;
; Hint: While-loop pattern: test condition first, then body, then loop back
; Hint: Continue while R0 < 100 (use CMP and JC)

section .code

; TODO: Load 3 into R0
; TODO: Load 100 into R1 (threshold)
loop:
    ; TODO: Compare R0 with R1 (R0 - R1)
    ; TODO: If R0 >= R1 (no carry), jump to done
    ; TODO: Double R0 (use SHL or ADD R0, R0)
    ; TODO: Jump back to loop
done:
    ; TODO: Halt (R0 should be >= 100, specifically 192)
