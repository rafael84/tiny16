; Exercise 12: Compare Numbers
; Level: 2 - Control Flow
;
; Goal: Load two numbers (5 and 8) and store the maximum in R0
;       Use CMP and conditional jumps
;
; Instructions to use: LOADI, CMP, JC, MOV, HALT
; Expected result: R0 = 8 (the maximum)
;
; Hint: CMP R0, R1 computes R0 - R1 and sets flags
; Hint: If R0 < R1, the C flag is set (borrow occurred)
; Hint: JC (Jump if Carry) can be used for "less than" comparisons

section .code

; TODO: Load 5 into R0
; TODO: Load 8 into R1
; TODO: Compare R0 with R1
; TODO: If R0 < R1 (C=1), jump to r1_larger
; TODO: Jump to done (R0 >= R1, R0 is max)
r1_larger:
    ; TODO: Move R1 to R0 (R1 is the max)
done:
    ; TODO: Halt (R0 should be 8)
