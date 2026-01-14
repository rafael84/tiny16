; Exercise 14: For Loop
; Level: 2 - Control Flow
;
; Goal: Sum numbers from 1 to 5 using a for-loop pattern
;       R0 = accumulator (sum), R1 = counter, R2 = limit
;
; Instructions to use: LOADI, ADD, INC, CMP, JZ, JMP, HALT
; Expected result: R0 = 15 (1+2+3+4+5)
;
; Hint: For-loop pattern: initialize counter, loop body, increment, test, repeat
; Hint: Sum should include: 1+2+3+4+5 = 15

section .code

; TODO: Load 0 into R0 (sum accumulator)
; TODO: Load 1 into R1 (counter, start at 1)
; TODO: Load 6 into R2 (limit, stop when counter reaches 6)
loop:
    ; TODO: Add R1 to R0 (accumulate sum)
    ; TODO: Increment R1 (next number)
    ; TODO: Compare R1 with R2
    ; TODO: If not equal, jump back to loop
; TODO: Halt (R0 should be 15)
