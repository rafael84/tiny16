; Exercise 23: Array Sum
; Level: 3 - Memory & Data Structures
; Goal: Sum all elements in an array using a loop
;       Array: [10, 20, 30, 40, 50] at 0x4000
; Instructions to use: LOADI, LOAD with [PAIR]+, ADD, DEC, JNZ, HALT
; Expected result: R0 = 150 (sum of all elements)
; Hint: Use post-increment [R6:R7]+ to traverse array
; Hint: Use R0 for accumulator, R1 for loop counter
; Hint: Pattern: load element, add to sum, decrement counter, repeat

section .code

loadi   r0, 0
loadi   r1, 5
loadi   r6, 0x40
loadi   r7, 0x00
loop:
    load    r2, [r6:r7]+
    add     r0, r2
    dec     r1
    jnz     loop
halt

section .data

DB 10, 20, 30, 40, 50
