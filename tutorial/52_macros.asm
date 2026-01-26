; Tutorial 52: Macros
; Level: 8 - Assembler Power Features
;
; Goal: Use a macro to add immediate values to a register
; Expected result: R0 = 22
;
; Hint: Macros are expanded before assembly (pass 0)
; Hint: Use a scratch register inside the macro (R7 here)

.macro ADDI reg, imm
    LOADI R7, imm
    ADD   reg, R7
.endmacro

section .code

loadi r0, 10
ADDI r0, 5
ADDI r0, 7
halt
