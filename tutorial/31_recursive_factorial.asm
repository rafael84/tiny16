; Exercise 31: Recursive Factorial
; Level: 4 - Stack & Subroutines
;
; Goal: Compute factorial(5) = 120 using recursion
;       factorial(n) = n * factorial(n-1), base case: factorial(0) = 1
;
; Instructions to use: LOADI, CALL, RET, DEC, CMP, JZ, PUSH, POP, HALT
; Expected result: R0 = 120 (5! = 5*4*3*2*1)
;
; Hint: This is ADVANCED - requires preserving R0 across recursive calls
; Hint: Pattern: save R0, decrement, recurse, restore R0, multiply
; Hint: Base case: if R0 <= 1, return 1
; Hint: Stack depth = n levels

section .code

; TODO: Load 5 into R0
; TODO: Call factorial
; TODO: Halt (R0 should be 120)

factorial:
    ; TODO: Compare R0 with 1
    ; TODO: If R0 <= 1, load 1 into R0 and return (base case)
    ; TODO: Push R0 (save current n)
    ; TODO: Decrement R0 (n-1)
    ; TODO: Call factorial recursively (computes (n-1)!)
    ; TODO: Pop R1 (restore original n)
    ; TODO: Multiply: result = n * factorial(n-1)
    ;      (Use repeated addition in a loop)
    ; TODO: Return (result in R0)
