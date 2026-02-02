; Exercise 29: Nested Subroutine Calls
; Level: 4 - Stack & Subroutines
;
; Goal: Call a function that calls another function (nested calls)
;       outer() calls inner(), demonstrating call stack management
;
; Instructions to use: LOADI, CALL, RET, INC, HALT
; Expected result: R0 = 11 (10 + 1 from inner function)
;
; Hint: Each CALL pushes return address on stack
; Hint: Each RET pops correct return address
; Hint: Stack manages multiple return addresses automatically

section .code

call outer          ; Call outer
halt                ; Halt (R0 should be 11)

outer:
    loadi   r0, 10  ; Load 10 into R0
    call    inner   ; Call inner (nested call)
    ret             ; Return to main

inner:
    inc     r0      ; Increment R0 (R0 = 11)
    ret             ; Return to outer
