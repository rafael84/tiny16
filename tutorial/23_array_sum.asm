; Exercise 23: Array Sum
; Level: 3 - Memory & Data Structures
;
; Goal: Sum all elements in an array using a loop
;       Array: [10, 20, 30, 40, 50] at 0x4000
;
; Instructions to use: LOADI, LOAD with [PAIR]+, ADD, DEC, JNZ, HALT
; Expected result: R0 = 150 (sum of all elements)
;
; Hint: Use post-increment [R6:R7]+ to traverse array
; Hint: Use R0 for accumulator, R1 for loop counter
; Hint: Pattern: load element, add to sum, decrement counter, repeat

section .code

loadi   r0, 0               ; Load 0 into R0 (sum accumulator)
loadi   r1, 5               ; Load 5 into R1 (array length / loop counter)
loadi   r6, 0x40            ; Load 0x40 into R6 (array address high)
loadi   r7, 0x00            ; Load 0x00 into R7 (array address low)
loop:
    load    r2, [r6:r7]+    ; Load from [R6:R7]+ into R2
    add     r0, r2          ; Add R2 to R0
    dec     r1              ; Decrement R1
    jnz     loop            ; Jump to loop if R1 != 0
halt                        ; Halt (R0 should be 150)

section .data

DB 10, 20, 30, 40, 50
