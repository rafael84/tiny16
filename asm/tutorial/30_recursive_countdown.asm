; Exercise 30: Recursive Countdown
; Level: 4 - Stack & Subroutines
;
; Goal: Implement countdown(n) recursively
;       countdown(5) -> 5, 4, 3, 2, 1, 0 stored in memory
;
; Instructions to use: LOADI, CALL, RET, DEC, OR, JZ, JMP, HALT
; Expected result: R0 = 0 (countdown complete)
;
; Hint: Base case: if R0 == 0, return
; Hint: Recursive case: decrement R0, call countdown
; Hint: Each recursive call uses stack space
; Hint: This demonstrates recursion fundamentals

section .code

loadi   r0, 5           ; Load 5 into R0
call    countdown       ; Call countdown
halt                    ; Halt (R0 should be 0)

countdown:
    or      r0, r0      ; Check if R0 == 0 (sets Z flag if R0 is zero)
    jz      done        ; If R0 == 0, return (base case)
    dec     r0          ; Decrement R0
    jmp     countdown   ; Call countdown recursively
    done:   ret         ; Return
