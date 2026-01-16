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

loadi r0, 0     ; Load 0 into R0 (counter)
loadi r1, 10    ; Load 10 into R1 (target)
loop:
    cmp r0, r1  ; Compare R0 with R1
    jz  done    ; If equal (Z=1), jump to done
    inc r0      ; Increment R0
    jmp loop    ; Jump back to loop
done:
    halt        ; Halt (R0 should be 10)
