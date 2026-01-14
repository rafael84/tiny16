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

; TODO: Load 5 into R0 (counter)
; TODO: Load 0x40 into R6, 0x00 into R7 (source address 0x4000)
; TODO: Load 0x40 into R4, 0x10 into R5 (dest address 0x4010)
loop:
    ; TODO: Load byte from [R6:R7]+ into R1
    ; TODO: Store R1 to [R4:R5]+
    ; TODO: Decrement R0
    ; TODO: Jump to loop if R0 != 0
; TODO: Halt

section .data

source: DB 10, 20, 30, 40, 50
        TIMES 11 DB 0      ; Padding to reach offset 0x10
dest:   TIMES 5 DB 0       ; Destination buffer
