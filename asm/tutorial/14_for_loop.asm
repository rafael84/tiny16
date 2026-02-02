; Exercise 14: For Loop
; Level: 2 - Control Flow
;
; Goal: Sum numbers from 1 to 5 using a for-loop pattern
;       R0 = accumulator (sum), R1 = counter, R2 = limit
;
; Instructions to use: LOADI, ADD, INC, CMP, JNZ, HALT
; Expected result: R0 = 15 (1+2+3+4+5)
;
; Hint: For-loop pattern: initialize counter, loop body, increment, test, repeat
; Hint: Sum should include: 1+2+3+4+5 = 15

section .code

loadi r0, 0     ; Load 0 into R0 (sum accumulator)
loadi r1, 1     ; Load 1 into R1 (counter, start at 1)
loadi r2  6     ; Load 6 into R2 (limit, stop when counter reaches 6)
loop:
    add r0, r1  ; Add R1 to R0 (accumulate sum)
    inc r1      ; Increment R1 (next number)
    cmp r1, r2  ; Compare R1 with R2
    jnz loop    ; If not equal, jump back to loop
halt            ; Halt (R0 should be 15)
