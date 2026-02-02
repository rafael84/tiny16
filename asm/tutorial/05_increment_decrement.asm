; Exercise 05: Increment and Decrement
; Level: 1 - Fundamentals
;
; Goal: Load 10 into R0, increment it twice, then decrement it once
;
; Instructions to use: LOADI, INC, DEC, HALT
; Expected result: R0 = 11
;
; Hint: INC R increments register by 1 (R = R + 1)
; Hint: DEC R decrements register by 1 (R = R - 1)
; Hint: INC and DEC set Z flag if result is 0, but always clear C flag

section .code

        loadi r0, 10    ; Load 10 into R0
times 2 inc r0          ; Increment R0 (R0 becomes 11)
                        ; Increment R0 again (R0 becomes 12)
        dec r0          ; Decrement R0 (R0 becomes 11)
        halt            ; Halt the program
