; Exercise 17: Load from Memory
; Level: 3 - Memory & Data Structures
;
; Goal: Load a byte from memory address 0x4000 into R0
;       Use register pair R6:R7 to form the 16-bit address
;
; Instructions to use: LOADI, LOAD, HALT
; Expected result: R0 = 42 (the value at 0x4000)
;
; Hint: Register pairs form 16-bit addresses: ADDR = (Rhigh << 8) | Rlow
; Hint: R6:R7 is the conventional pair for memory operations
; Hint: LOAD R, [PAIR] loads byte from memory into register

section .code

loadi r6, 0x40      ; Load 0x40 into R6 (high byte of address)
loadi r7, 0x00      ; Load 0x00 into R7 (low byte of address)
load r0, [r6:r7]    ; Load from address [R6:R7] into R0
halt                ; Halt (R0 should be 42)

section .data

DB 42  ; Byte at address 0x4000
