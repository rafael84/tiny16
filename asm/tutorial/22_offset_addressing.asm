; Exercise 22: Offset Addressing for Structs
; Level: 3 - Memory & Data Structures
; Goal: Access struct-like data using offset addressing
;       Struct at 0x4000: {id: 1, x: 50, y: 75, color: 255}
; Instructions to use: LOADI, LOAD with offsets, HALT
; Expected result: R0=1, R1=50, R2=75, R3=255
; Hint: Offset addressing is perfect for struct field access
; Hint: Base address stays constant, offsets reach different fields
; Hint: [PAIR + N] doesn't modify PAIR

section .code

loadi   r6, 0x40
loadi   r7, 0x00
load    r0, [r6:r7+0]
load    r1, [r6:r7+1]
load    r2, [r6:r7+2]
load    r3, [r6:r7+3]
halt

section .data

entity:
    DB 1
    DB 50
    DB 75
    DB 255
