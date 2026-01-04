LOADI R0, 3      ; 10 00 03
loop:
    DEC R0       ; 23 00 00
    JNZ loop     ; 32 00 03
    HALT         ; FF 00 00
