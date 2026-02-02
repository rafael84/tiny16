; Exercise 08: Halve a Number
; Level: 1 - Fundamentals
;
; Goal: Load 20 into R0, then halve it to 10
;
; Instructions to use: LOADI, SHR, HALT
; Expected result: R0 = 10
;
; Hint: SHR R (shift right) divides by 2 (rounds down)
; Hint: SHR sets C flag if bit 0 was 1 before shift (odd number)

section .code

loadi r0, 20    ; Load 20 into R0
shr r0          ; Halve R0 using SHR (shift right)
halt            ; Halt the program
