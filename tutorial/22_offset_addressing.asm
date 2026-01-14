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

; TODO: Load 0x40 into R6 (struct base address high)
; TODO: Load 0x00 into R7 (struct base address low)
; TODO: Load field 0 (id) from [R6:R7 + 0] into R0
; TODO: Load field 1 (x) from [R6:R7 + 1] into R1
; TODO: Load field 2 (y) from [R6:R7 + 2] into R2
; TODO: Load field 3 (color) from [R6:R7 + 3] into R3
; TODO: Halt

section .data

entity:
    DB 1      ; offset 0: id
    DB 50     ; offset 1: x position
    DB 75     ; offset 2: y position
    DB 255    ; offset 3: color (white)
