; Exercise 36: Small-value Multiplication (Shift-and-Add)
; Level: 5 - Advanced Arithmetic
; Goal: Multiply 1 * 255 = 255 using shift-and-add algorithm
;       Repeated addition would need 255 loops; this needs only 8
; Instructions to use: LOADI, ADD, SHL, SHR, AND, JZ, JMP, HALT
; Expected result: R0 = 255
; How it works: Any number is a sum of powers of 2 (its binary representation)
;   1 × 255 = 1 × (128 + 64 + 32 + 16 + 8 + 4 + 2 + 1)
;           = 1×2⁷ + 1×2⁶ + 1×2⁵ + 1×2⁴ + 1×2³ + 1×2² + 1×2¹ + 1×2⁰
; Algorithm: For each bit in multiplier (right to left):
;   - If bit is 1: add multiplicand to result
;   - Double multiplicand (shift left) → next power of 2
;   - Halve multiplier (shift right) → next bit
; Example with 12 × 5 = 60:
;   5 = 0b101 (bits 0 and 2 are set) = 4 + 1
;   Iter 1: bit=1, add 12×1  =  12  → result = 12
;   Iter 2: bit=0, skip             → result = 12
;   Iter 3: bit=1, add 12×4  =  48  → result = 60
;   Only 3 iterations instead of 5!
; Note: This is how hardware multipliers work
; Note: Limited to results that fit in 8-bit register (max 255)

section .code

loadi   r1, 1
loadi   r2, 255
loadi   r0, 0
loop:
    xor     r3, r3
    cmp     r2, r3
    jz      done

    loadi   r3, 1
    and     r3, r2
    jz      shift

    add     r0, r1
shift:
    shl     r1
    shr     r2
    jmp     loop
done:
    halt
