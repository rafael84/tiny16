; Exercise 20: Post-Increment Addressing
; Level: 3 - Memory & Data Structures
;
; Goal: Read first 3 elements of an array using post-increment
;       Array: [5, 10, 15, 20, 25] at 0x4000
;
; Instructions to use: LOADI, LOAD with [PAIR]+, HALT
; Expected result: R0=5, R1=10, R2=15, R6:R7=0x4003
;
; Hint: [PAIR]+ means: access memory at PAIR, then increment PAIR by 1
; Hint: Post-increment automatically advances to next array element
; Hint: This is the most efficient way to traverse arrays

section .code

; TODO: Load 0x40 into R6 (array base high)
; TODO: Load 0x00 into R7 (array base low)
; TODO: Load from [R6:R7]+ into R0 (reads 5, R6:R7 becomes 0x4001)
; TODO: Load from [R6:R7]+ into R1 (reads 10, R6:R7 becomes 0x4002)
; TODO: Load from [R6:R7]+ into R2 (reads 15, R6:R7 becomes 0x4003)
; TODO: Halt

section .data

DB 5, 10, 15, 20, 25
