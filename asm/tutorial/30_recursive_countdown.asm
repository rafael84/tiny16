; Exercise 30: Recursive Countdown
; Level: 4 - Stack & Subroutines
; Goal: Implement countdown(n) recursively
;       countdown(5) -> 5, 4, 3, 2, 1, 0 stored in memory
; Instructions to use: LOADI, CALL, RET, DEC, OR, JZ, JMP, HALT
; Expected result: R0 = 0 (countdown complete)
; Hint: Base case: if R0 == 0, return
; Hint: Recursive case: decrement R0, call countdown
; Hint: Each recursive call uses stack space
; Hint: This demonstrates recursion fundamentals

section .code

loadi   r0, 5
call    countdown
halt

countdown:
    or      r0, r0
    jz      done
    dec     r0
    jmp     countdown
    done:   ret
