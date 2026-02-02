; Exercise 21: Post-Decrement Addressing
; Level: 3 - Memory & Data Structures
;
; Goal: Read array elements in reverse using post-decrement
;       Start at end of array (0x4004) and read backwards
;
; Instructions to use: LOADI, LOAD with [PAIR]-, HALT
; Expected result: R0=25, R1=20, R2=15
;
; Hint: [PAIR]- means: access memory at PAIR, then decrement PAIR by 1
; Hint: Useful for reverse traversal, stack operations
; Hint: Array at 0x4000: [5, 10, 15, 20, 25], last element at 0x4004

section .code

loadi   r6, 0x40        ; Load 0x40 into R6 (high byte)
loadi   r7, 0x04        ; Load 0x04 into R7 (low byte - point to last element)
load    r0, [r6:r7]-    ; Load from [R6:R7]- into R0 (reads 25, R6:R7 becomes 0x4003)
load    r1, [r6:r7]-    ; Load from [R6:R7]- into R1 (reads 20, R6:R7 becomes 0x4002)
load    r2, [r6:r7]-    ; Load from [R6:R7]- into R2 (reads 15, R6:R7 becomes 0x4001)
halt                    ; Halt

section .data

DB 5, 10, 15, 20, 25
