; Exercise 32: Stack Frame (Local Variables)
; Level: 4 - Stack & Subroutines
;
; Goal: Create a function with local variables using stack frame
;       Allocate space on stack for locals, use them, then deallocate
;
; Instructions to use: LOADI, PUSH, POP, CALL, RET, STORE, LOAD, HALT
; Expected result: R0 = 15 (sum of local variables)
;
; Hint: Stack frame pattern:
;       1. PUSH registers to save them
;       2. Reserve stack space for locals (conceptually)
;       3. Use locals (via stack pointer calculations)
;       4. Deallocate (adjust SP if needed)
;       5. POP registers to restore
;       6. RET
; Hint: This is advanced but foundational for compilers

section .code

; TODO: Call compute
; TODO: Halt (R0 should be 15)

compute:
    ; Simulate local variables: a=5, b=10
    ; TODO: Load 5 into R0 (local a)
    ; TODO: Load 10 into R1 (local b)
    ; TODO: Add R1 to R0 (R0 = a + b = 15)
    ; TODO: Return
    ; Note: For true stack frame, would PUSH/POP and use [SP + offset]
