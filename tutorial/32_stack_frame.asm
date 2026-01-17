; Exercise 32: Stack-Based Parameters with Frame Pointer
; Level: 4 - Stack & Subroutines
;
; Goal: Pass three arguments via the stack to a function
;       Function uses frame pointer to access arguments, computes sum
;       Call sum(5, 10, 15) and get R0 = 30
;
; Instructions to use: LOADI, PUSH, MOVSPR, LOAD, ADD, CALL, RET, HALT
; Expected result: R0 = 30
;
; Hint: Push arguments in reverse order: arg3, arg2, arg1
; Hint: Function uses MOVSPR to get frame pointer (SP into R6:R7)
; Hint: Access arguments via offset: [R6:R7 + offset]
; Hint: This demonstrates standard frame-pointer calling convention
;
; Stack layout after CALL:
;   Higher addresses
;   [SP + 4] = arg3 (15) - first pushed
;   [SP + 3] = arg2 (10)
;   [SP + 2] = arg1 (5)  - last pushed, closest to return addr
;   [SP + 1] = return address low byte
;   [SP + 0] = return address high byte <- SP points here
;   Lower addresses

section .code

; Push arguments in order: arg1, arg2, arg3
loadi   r0, 5
push    r0          ; arg1
loadi   r0, 10
push    r0          ; arg2
loadi   r0, 15
push    r0          ; arg3
call    sum_three
halt

sum_three:
    movspr  r6:r7               ; R6:R7 = SP (frame pointer)
    load    r2, [r6:r7 + 3]     ; arg3 (15) at SP+3 (last pushed)
    load    r1, [r6:r7 + 4]     ; arg2 (10) at SP+4
    load    r0, [r6:r7 + 5]     ; arg1 (5) at SP+5 (first pushed)
    add     r0, r1              ; R0 = 5 + 10 = 15
    add     r0, r2              ; R0 = 15 + 15 = 30
    ret
