; Exercise 22: Offset Addressing for Structs
; Level: 3 - Memory & Data Structures
;
; Goal: Access struct-like data using offset addressing
;       Struct at 0x4000: {id: 1, x: 50, y: 75, color: 255}
;
; Instructions to use: LOADI, LOAD with offsets, HALT
; Expected result: R0=1, R1=50, R2=75, R3=255
;
; Hint: Offset addressing is perfect for struct field access
; Hint: Base address stays constant, offsets reach different fields
; Hint: [PAIR + N] doesn't modify PAIR

section .code

loadi   r6, 0x40        ; Load 0x40 into R6 (struct base address high)
loadi   r7, 0x00        ; Load 0x00 into R7 (struct base address low)
load    r0, [r6:r7+0]   ; Load field 0 (id) from [R6:R7 + 0] into R0
load    r1, [r6:r7+1]   ; Load field 1 (x) from [R6:R7 + 1] into R1
load    r2, [r6:r7+2]   ; Load field 2 (y) from [R6:R7 + 2] into R2
load    r3, [r6:r7+3]   ; Load field 3 (color) from [R6:R7 + 3] into R3
halt                    ; Halt

section .data

entity:
    DB 1      ; offset 0: id
    DB 50     ; offset 1: x position
    DB 75     ; offset 2: y position
    DB 255    ; offset 3: color (white)
