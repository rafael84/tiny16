; Exercise 12: Array Copy
; Goal: Copy array from source to destination in memory
; Source array: [1, 2, 3, 4, 5] at 0x4000
; Destination: 0x4010
; Instructions to use: LOADI, LOAD, STORE, INC, DEC, JNZ
; Expected result: Destination contains [1, 2, 3, 4, 5]

section .code

; TODO: Your code here
; Strategy:
; - Use R6:R7 for source address (starts at 0x4000)
; - Use R4:R5 for destination address (starts at 0x4010)
; - Use R3 as counter (5 elements)
; - Loop: LOAD from source, switch to dest address, STORE, increment both, repeat
; Hint: You'll need to juggle addresses between R6:R7 and R4:R5
; Challenge: Can you do this without using R4:R5?

section .data

source: DB 1, 2, 3, 4, 5
dest:   DB 0, 0, 0, 0, 0  ; Will be filled by your code

HALT
