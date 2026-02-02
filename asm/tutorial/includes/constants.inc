; Shared constants and macros for tutorial 53

BASE_VALUE = 12

.macro ADD_BASE reg
    LOADI R6, BASE_VALUE
    ADD   reg, R6
.endmacro
