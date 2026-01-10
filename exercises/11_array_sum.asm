; Exercise 11: Array Sum
; Goal: Sum all elements in an array stored in memory
; Array: [10, 20, 30, 40, 50] at address 0x2000
; Instructions to use: LOADI, LOAD, ADD, INC, SUB, JZ
; Expected result: R0 = 150

section .code

; TODO: Your code here
; Strategy:
; - Set R6:R7 to point to array start (0x2000)
; - Use R0 as sum accumulator (start at 0)
; - Use R1 as counter (5 elements)
; - Loop: LOAD value, ADD to sum, increment address, decrement counter
; Hint: R6:R7 forms the 16-bit address for LOAD/STORE
; Hint: Increment R7 to move to next byte (watch for overflow to R6!)

section .data

array: DB 10, 20, 30, 40, 50

HALT
