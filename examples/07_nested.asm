start: LOADI R0, 2  ; 10 00 02
outer: DEC R0       ; 23 00 00
       JZ end       ; 31 00 15
       LOADI R1, 1  ; 10 01 01
inner: DEC R1       ; 23 01 00
       JNZ inner    ; 32 00 0C
       JMP outer    ; 30 00 03
end:   HALT         ; FF 00 00
