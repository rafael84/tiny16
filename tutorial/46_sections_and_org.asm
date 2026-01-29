; Tutorial 46: Sections and ORG
; Level: 7 - Assembler Power Features
;
; Goal: Read array at 0x4020, compute sum, store at 0x4100
; Expected: Memory[0x4100] = 0x96 (150 = sum of 10,20,30,40,50)
;
; Concepts: Multiple ORG directives, expressions in immediates

ARRAY_ADDR  = 0x4020
RESULT_ADDR = 0x4100
ARRAY_LEN   = 5

section .code

loadi   r6, ARRAY_ADDR >> 8
loadi   r7, ARRAY_ADDR & 0xFF
loadi   r2, ARRAY_LEN

xor     r0, r0
loop:
    load    r1, [r6:r7]+
    add     r0, r1
    dec     r2
    jnz     loop

loadi   r6, RESULT_ADDR >> 8
loadi   r7, RESULT_ADDR & 0xFF
store   r0, [r6:r7]

halt

section .data

org ARRAY_ADDR
array:  db 10, 20, 30, 40, 50

org RESULT_ADDR
result: db 0
