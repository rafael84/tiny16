; Exercise 12: Compare Numbers
; Level: 2 - Control Flow
;
; Goal: Load two numbers (5 and 8) and store the maximum in R0
;       Use CMP and conditional jumps
;
; Instructions to use: LOADI, CMP, JC, JMP, MOV, HALT
; Expected result: R0 = 8 (the maximum)
;
; Hint: CMP R0, R1 computes R0 - R1 and sets flags
; Hint: If R0 < R1, the C flag is set (borrow occurred)
; Hint: JC (Jump if Carry) can be used for "less than" comparisons

section .code

loadi r0, 5     ; Load 5 into R0
loadi r1, 8     ; Load 8 into R1
cmp r0, r1      ; Compare R0 with R1
jc r1_larger    ; If R0 < R1 (C=1), jump to r1_larger
jmp done        ; Jump to done (R0 >= R1, R0 is max)
r1_larger:
    mov r0, r1  ; Move R1 to R0 (R1 is the max)
done:
    halt        ; Halt (R0 should be 8)
