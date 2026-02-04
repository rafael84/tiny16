; Exercise 16: Nested Loops
; Level: 2 - Control Flow
; Goal: Implement nested loops to count iterations
;       Outer loop: 3 times, Inner loop: 4 times each
;       Count total iterations in R0 (should be 3 * 4 = 12)
; Instructions to use: LOADI, INC, DEC, JNZ, HALT
; Expected result: R0 = 12
; Hint: Use R1 for outer loop counter, R2 for inner loop counter
; Hint: Must reload R2 for each outer iteration
; Hint: Nested structure: outer_loop { inner_loop { count++ } }

section .code

loadi r0, 0
loadi r1, 3
outer_loop:
    loadi r2, 4
    inner_loop:
        inc r0
        dec r2
        jnz inner_loop
    dec r1
    jnz outer_loop
halt
