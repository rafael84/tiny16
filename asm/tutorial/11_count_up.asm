; Exercise 11: Count Up
; Level: 2 - Control Flow
; Goal: Count from 0 up to 10 using INC and CMP
;       Stop when R0 equals 10
; Instructions to use: LOADI, INC, CMP, JZ, JMP, HALT
; Expected result: R0 = 10
; Hint: CMP R0, R1 compares R0 with R1, sets Z flag if equal
; Hint: JZ (Jump if Zero) jumps when Z=1 (values are equal)
; Hint: Use JMP to continue loop if not equal

section .code

loadi r0, 0
loadi r1, 10
loop:
    cmp r0, r1
    jz  done
    inc r0
    jmp loop
done:
    halt
