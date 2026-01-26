; Tutorial 46: Sections and ORG
; Level: 7 - Assembler Power Features
;
; Goal: Place data at a specific address with ORG and read it back
; Expected result: R0 = 0x2A
;
; Hint: ORG is only valid in the .data section
; Hint: Use expressions to split 16-bit addresses into high/low bytes

DATA_ADDR = 0x4010

section .code

loadi r6, DATA_ADDR >> 8
loadi r7, DATA_ADDR & 0xFF
load  r0, [r6:r7]
halt

section .data

org DATA_ADDR
value: db 0x2A
