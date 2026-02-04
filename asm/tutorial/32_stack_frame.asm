; Exercise 32: Stack-Based Parameters with Frame Pointer
; Level: 4 - Stack & Subroutines
; Goal: Pass three arguments via the stack to a function
;       Function uses frame pointer to access arguments, computes sum
;       Also demonstrates local variables with negative offsets
;       Call sum(5, 10, 15) and get R0 = 30
; Instructions to use: LOADI, PUSH, MOVSPR, LOAD, STORE, ADD, CALL, RET, HALT
; Expected result: R0 = 30
; Hint: Push arguments in reverse order: arg3, arg2, arg1
; Hint: Function uses MOVSPR to get frame pointer (SP into R6:R7)
; Hint: Access arguments via POSITIVE offset: [R6:R7 + offset]
; Hint: Access local variables via NEGATIVE offset: [R6:R7 - offset]
; Hint: Signed offsets range from -128 to +127
; Stack layout after MOVSPR and local allocation:
;   Higher addresses
;   [FP + 5] = arg1 (5)  - first pushed
;   [FP + 4] = arg2 (10)
;   [FP + 3] = arg3 (15) - last pushed
;   [FP + 2] = return address high byte
;   [FP + 1] = return address low byte
;   [FP + 0] = SP points here after CALL <- FP saved here
;   [FP - 1] = local1 (temporary sum)    <- pushed after MOVSPR
;   Lower addresses

section .code

; Push arguments in order: arg1, arg2, arg3
loadi   r0, 5
push    r0
loadi   r0, 10
push    r0
loadi   r0, 15
push    r0
call    sum_three
halt

sum_three:
    movspr  r6:r7
    loadi   r0, 0
    push    r0

    load    r0, [r6:r7 + 5]
    load    r1, [r6:r7 + 4]
    add     r0, r1

    store   r0, [r6:r7 - 1]

    load    r0, [r6:r7 - 1]
    load    r2, [r6:r7 + 3]
    add     r0, r2

    pop     r1
    ret
