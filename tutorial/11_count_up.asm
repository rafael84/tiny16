; Exercise 11: Count Up
; Level: 2 - Control Flow
;
; Goal: Count from 0 up to 10 using INC and CMP
;       Stop when R0 equals 10
;
; Instructions to use: LOADI, INC, CMP, JZ, JMP, HALT
; Expected result: R0 = 10
;
; Hint: CMP R0, R1 compares R0 with R1, sets Z flag if equal
; Hint: JZ (Jump if Zero) jumps when Z=1 (values are equal)
; Hint: Use JMP to continue loop if not equal

section .code

; TODO: Load 0 into R0 (counter)
; TODO: Load 10 into R1 (target)
loop:
    ; TODO: Compare R0 with R1
    ; TODO: If equal (Z=1), jump to done
    ; TODO: Increment R0
    ; TODO: Jump back to loop
done:
    ; TODO: Halt (R0 should be 10)
