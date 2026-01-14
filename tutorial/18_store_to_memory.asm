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

; TODO: Load 0x40 into R6 (high byte)
; TODO: Load 0x00 into R7 (low byte)
; TODO: Load 99 into R0
; TODO: Store R0 to address [R6:R7]
; TODO: Load from [R6:R7] into R1 to verify
; TODO: Halt (R1 should be 99)

section .data

DB 0  ; Reserve space at 0x4000 (initially 0)
