; Exercise 30: Recursive Countdown
; Level: 4 - Stack & Subroutines
;
; Goal: Implement countdown(n) recursively
;       countdown(5) -> 5, 4, 3, 2, 1, 0 stored in memory
;
; Instructions to use: LOADI, CALL, RET, DEC, JZ, HALT
; Expected result: R0 = 0 (countdown complete)
;
; Hint: Base case: if R0 == 0, return
; Hint: Recursive case: decrement R0, call countdown
; Hint: Each recursive call uses stack space
; Hint: This demonstrates recursion fundamentals

section .code

; TODO: Load 5 into R0
; TODO: Call countdown
; TODO: Halt (R0 should be 0)

countdown:
    ; TODO: Check if R0 == 0 (compare with itself or check Z flag)
    ; TODO: If R0 == 0, return (base case)
    ; TODO: Decrement R0
    ; TODO: Call countdown recursively
    ; TODO: Return
