LOADI R0, 1      ; 10 00 01
DEC   R0         ; 23 00 00
JZ    done       ; 31 00 0C
LOADI R1, 42     ; 10 01 2A
done:
    HALT         ; FF 00 00
