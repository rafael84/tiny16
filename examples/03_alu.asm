LOADI R0, 10     ; 10 00 0A
LOADI R1, 3      ; 10 01 03
ADD  R0, R1      ; 20 00 01
SUB  R0, R1      ; 21 00 01
AND  R0, R1      ; 24 00 01
OR   R0, R1      ; 25 00 01
XOR  R0, R1      ; 26 00 01
INC  R0          ; 22 00 00
DEC  R0          ; 23 00 00
HALT             ; FF 00 00
