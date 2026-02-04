; Exercise 21: Post-Decrement Addressing
; Level: 3 - Memory & Data Structures
; Goal: Read array elements in reverse using post-decrement
;       Start at end of array (0x4004) and read backwards
; Instructions to use: LOADI, LOAD with [PAIR]-, HALT
; Expected result: R0=25, R1=20, R2=15
; Hint: [PAIR]- means: access memory at PAIR, then decrement PAIR by 1
; Hint: Useful for reverse traversal, stack operations
; Hint: Array at 0x4000: [5, 10, 15, 20, 25], last element at 0x4004

section .code

loadi   r6, 0x40
loadi   r7, 0x04
load    r0, [r6:r7]-
load    r1, [r6:r7]-
load    r2, [r6:r7]-
halt

section .data

DB 5, 10, 15, 20, 25
