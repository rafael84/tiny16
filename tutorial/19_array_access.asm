; Exercise 19: Array Access
; Level: 3 - Memory & Data Structures
;
; Goal: Access the 3rd element (index 2) of an array using offset addressing
;       Array: [10, 20, 30, 40, 50] at 0x4000
;
; Instructions to use: LOADI, LOAD with offset, HALT
; Expected result: R0 = 30 (array[2])
;
; Hint: LOAD R, [PAIR + offset] accesses memory at (PAIR + offset)
; Hint: Array indices: 0=10, 1=20, 2=30, 3=40, 4=50
; Hint: Offset addressing doesn't modify the register pair

section .code

; TODO: Load 0x40 into R6 (array base address high)
; TODO: Load 0x00 into R7 (array base address low)
; TODO: Load from [R6:R7 + 2] into R0 (3rd element, offset 2)
; TODO: Halt (R0 should be 30)

section .data

array: DB 10, 20, 30, 40, 50
