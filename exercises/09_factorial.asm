; Exercise 9: Factorial
;
; Goal: Calculate N! = N × (N-1) × ... × 1
; Example: 5! = 5 × 4 × 3 × 2 × 1 = 120
; Note: Result must fit in 8 bits, so max is 5! = 120
; Instructions to use: LOADI, DEC, JZ, multiplication loop
; Expected result: R0 = 120 (for N=5)

section .code

loadi   R0, 5   ; R0 = result
mov     R1, R0  ; R1 = counter
dec     R1      ; R1 = N - 1

fact:   mov R2, R1  ; R2 = multiplier
        mov R3, R0  ; R3 = multiplicand
        loadi R0, 0 ; RESET accumulator
        mult:   add R0, R3
                dec R2
                jnz mult
        dec R1
        jnz fact
halt

;   5 x 4 = 5 + 5 + 5 + 5 = 20
;  20 x 3 = 20 + 20 + 20 = 60
;  60 x 2 = 60 + 60 = 120
; 120 x 1 = 120
