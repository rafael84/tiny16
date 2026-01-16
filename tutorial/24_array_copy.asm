; Exercise 24: Array Copy
; Level: 3 - Memory & Data Structures
;
; Goal: Copy 5 bytes from source (0x4000) to destination (0x4010)
;       Use two register pairs: R6:R7 for source, R4:R5 for dest
;
; Instructions to use: LOADI, LOAD, STORE, post-increment, DEC, JNZ, HALT
; Expected result: Bytes at 0x4010-0x4014 = [10, 20, 30, 40, 50]
;
; Hint: Use R6:R7 for source, R4:R5 for destination
; Hint: Post-increment on both pairs: [R6:R7]+ and [R4:R5]+
; Hint: Load from source, store to destination, repeat

section .code

loadi   r0, 5               ; Load 5 into R0 (counter)
loadi   r6, 0x40            ; Load 0x40 into R6, 0x00 into R7 (source address 0x4000)
loadi   r7, 0x00
loadi   r4, 0x40            ; Load 0x40 into R4, 0x10 into R5 (dest address 0x4010)
loadi   r5, 0x10
loop:
    load    r1, [r6:r7]+    ; Load byte from [R6:R7]+ into R1
    store   r1, [r4:r5]+    ; Store R1 to [R4:R5]+
    dec     r0              ; Decrement R0
    jnz     loop            ; Jump to loop if R0 != 0
halt                        ; Halt

section .data

source: DB 10, 20, 30, 40, 50
        TIMES 11 DB 0      ; Padding to reach offset 0x10
dest:   TIMES 5 DB 0       ; Destination buffer
