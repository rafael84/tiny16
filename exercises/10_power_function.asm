; Exercise 10: Power Function
;
; Goal: Calculate A^B (A to the power of B)
; Example: 2^5 = 32
; Instructions to use: LOADI, DEC, JZ, multiplication loop
; Expected result: R0 = 32 (for 2^5)
section .code

loadi r1, 2 ; r1 = base (a)
loadi r2, 5 ; r2 = exponent (b)
loadi r0, 1 ; r0 = result accumulator (a^0 = 1)

; force a flag update for r2
and r2, r2  ; r2 = r2 & r2. updates z flag.
jz done     ; jumps if b=0 (a^0)

exp_loop:
    ; multiply the current result (r0) by the base (r1)
    ; we do this by adding r0 to itself r1 times.

    mov r3, r0    ; r3 = current value to be added (multiplicand)
    loadi r0, 0   ; reset r0 to 0 to begin the summation
    mov r4, r1    ; r4 = counter for the multiplication loop (multiplier)

    mul_loop:
        add r0, r3    ; result = result + multiplicand
        dec r4        ; decrement inner counter
        jnz mul_loop  ; if r4 != 0, add again

        ; --- end of multiplication step ---

        dec r2        ; decrement exponent counter
        jnz exp_loop  ; if r2 != 0, perform next multiplication

done:
    halt
