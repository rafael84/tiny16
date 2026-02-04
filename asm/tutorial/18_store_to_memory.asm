; Exercise 18: Store to Memory
; Level: 3 - Memory & Data Structures
; Goal: Store the value 99 from R0 into memory at address 0x4000
;       Then load it back to verify
; Instructions to use: LOADI, STORE, LOAD, HALT
; Expected result: R1 = 99 (loaded back from memory)
; Hint: STORE R, [PAIR] stores register value to memory
; Hint: Cannot write to code segment (0x0010-0x3FFF)
; Hint: Data section starts at 0x4000

section .code

loadi   r6, 0x40
loadi   r7, 0x00
loadi   r0, 99
store   r0, [r6:r7]
load    r1, [r6:r7]
halt

section .data

DB 0
