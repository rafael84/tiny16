.macro CLEAR reg
    XOR reg, reg
.endmacro

section .code

START:
    CLEAR R0
    CLEAR R1
    HALT
