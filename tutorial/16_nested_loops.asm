; Exercise 16: Nested Loops
; Level: 2 - Control Flow
;
; Goal: Implement nested loops to count iterations
;       Outer loop: 3 times, Inner loop: 4 times each
;       Count total iterations in R0 (should be 3 * 4 = 12)
;
; Instructions to use: LOADI, INC, DEC, JNZ, HALT
; Expected result: R0 = 12
;
; Hint: Use R1 for outer loop counter, R2 for inner loop counter
; Hint: Must reload R2 for each outer iteration
; Hint: Nested structure: outer_loop { inner_loop { count++ } }

section .code

loadi r0, 0             ; Load 0 into R0 (total count)
loadi r1, 3             ; Load 3 into R1 (outer loop iterations)
outer_loop:
    loadi r2, 4         ; Load 4 into R2 (inner loop iterations - reload each time)
    inner_loop:
        inc r0          ; Increment R0 (count total iterations)
        dec r2          ; Decrement R2
        jnz inner_loop  ; If R2 != 0, jump to inner_loop
    dec r1              ; Decrement R1 (outer counter)
    jnz outer_loop      ; If R1 != 0, jump to outer_loop
halt                    ; Halt (R0 should be 12)
