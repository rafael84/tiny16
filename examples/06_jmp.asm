LOADI R0, 5      ; 10 00 05
JMP target       ; 30 00 09
LOADI R0, 99     ; 10 00 63
target:
    HALT         ; FF 00 00
