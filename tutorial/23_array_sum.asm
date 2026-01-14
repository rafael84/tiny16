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

; TODO: Load 0 into R0 (sum accumulator)
; TODO: Load 5 into R1 (array length / loop counter)
; TODO: Load 0x40 into R6 (array address high)
; TODO: Load 0x00 into R7 (array address low)
loop:
    ; TODO: Load from [R6:R7]+ into R2
    ; TODO: Add R2 to R0
    ; TODO: Decrement R1
    ; TODO: Jump to loop if R1 != 0
; TODO: Halt (R0 should be 150)

section .data

DB 10, 20, 30, 40, 50
