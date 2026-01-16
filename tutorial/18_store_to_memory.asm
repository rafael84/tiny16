; Exercise 18: Store to Memory
; Level: 3 - Memory & Data Structures
;
; Goal: Store the value 99 from R0 into memory at address 0x4000
;       Then load it back to verify
;
; Instructions to use: LOADI, STORE, LOAD, HALT
; Expected result: R1 = 99 (loaded back from memory)
;
; Hint: STORE R, [PAIR] stores register value to memory
; Hint: Cannot write to code segment (0x0010-0x3FFF)
; Hint: Data section starts at 0x4000

section .code

loadi   r6, 0x40    ; Load 0x40 into R6 (high byte)
loadi   r7, 0x00    ; Load 0x00 into R7 (low byte)
loadi   r0, 99      ; Load 99 into R0
store   r0, [r6:r7] ; Store R0 to address [R6:R7]
load    r1, [r6:r7] ; Load from [R6:R7] into R1 to verify
halt                ; Halt (R1 should be 99)

section .data

DB 0  ; Reserve space at 0x4000 (initially 0)
