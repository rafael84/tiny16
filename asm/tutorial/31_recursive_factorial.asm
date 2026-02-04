; Exercise 31: Recursive Factorial
; Level: 4 - Stack & Subroutines
; Goal: Compute factorial(5) = 120 using recursion
;       factorial(n) = n * factorial(n-1), base case: factorial(0) = 1
; Instructions to use: LOADI, CALL, RET, DEC, CMP, JZ, PUSH, POP, HALT
; Expected result: R0 = 120 (5! = 5*4*3*2*1)
; Hint: This is ADVANCED - requires preserving R0 across recursive calls
; Hint: Pattern: save R0, decrement, recurse, restore R0, multiply
; Hint: Base case: if R0 <= 1, return 1
; Hint: Stack depth = n levels

section .code

loadi   r0, 5
call    factorial
halt

factorial:
    loadi   r1, 1
    cmp     r0, r1
    jc      base_case
    jz      base_case

    push    r0
    dec     r0
    call    factorial
    pop     r1

    mov     r2, r0
    loadi   r0, 0
    multiply:
        add     r0, r2
        dec     r1
        jnz     multiply
        ret

    base_case:
        loadi   r0, 1
        ret
