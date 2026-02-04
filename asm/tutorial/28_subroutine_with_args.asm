; Exercise 28: Subroutine with Arguments
; Level: 4 - Stack & Subroutines
; Goal: Pass two arguments via registers to an add function
;       add_fn(R0=5, R1=7) returns R0=12
; Instructions to use: LOADI, CALL, RET, ADD, HALT
; Expected result: R0 = 12
; Hint: Calling convention: pass args in R0, R1; return result in R0
; Hint: Function adds R1 to R0 and returns
; Hint: This demonstrates parameter passing

section .code

loadi   r0, 5
loadi   r1, 7
call    add_fn
halt

add_fn:
    add r0, r1
    ret
