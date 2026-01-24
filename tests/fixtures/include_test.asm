.include "macros.inc"

section .code

START:
    CLEAR R0
    LOAD16 R1, 0xBF, 0x22
    HALT
