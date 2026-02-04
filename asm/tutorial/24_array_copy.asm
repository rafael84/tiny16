; Exercise 24: Array Copy
; Level: 3 - Memory & Data Structures
; Goal: Copy 5 bytes from source (0x4000) to destination (0x4010)
;       Use two register pairs: R6:R7 for source, R4:R5 for dest
; Instructions to use: LOADI, LOAD, STORE, post-increment, DEC, JNZ, HALT
; Expected result: Bytes at 0x4010-0x4014 = [10, 20, 30, 40, 50]
; Hint: Use R6:R7 for source, R4:R5 for destination
; Hint: Post-increment on both pairs: [R6:R7]+ and [R4:R5]+
; Hint: Load from source, store to destination, repeat

section .code

loadi   r0, 5
loadi   r6, 0x40
loadi   r7, 0x00
loadi   r4, 0x40
loadi   r5, 0x10
loop:
    load    r1, [r6:r7]+
    store   r1, [r4:r5]+
    dec     r0
    jnz     loop
halt

section .data

source: DB 10, 20, 30, 40, 50
        TIMES 11 DB 0
dest:   TIMES 5 DB 0
