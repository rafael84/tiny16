.macro LOAD16 dest, hi, lo
    LOADI R6, hi
    LOADI R7, lo
    LOAD dest, [R6:R7]
.endmacro

.macro CLEAR reg
    XOR reg, reg
.endmacro
