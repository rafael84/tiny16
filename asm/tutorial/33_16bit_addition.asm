; Tutorial 33: 16-bit Addition
; Level: 5 - Advanced Arithmetic
;
; Goal: Add two 16-bit numbers using ADC (add with carry)
;       0x01FF + 0x0002 = 0x0201 (511 + 2 = 513)
;
; Instructions to use: LOADI, ADD, ADC, HALT
; Expected result: R6:R7 = 0x0201 (R6 = 0x02, R7 = 0x01)
;
; Hint: ADC R1, R2 means R1 = R1 + R2 + C (add with carry flag)
; Hint: First add low bytes (sets C if overflow), then ADC high bytes
; Hint: 16-bit number stored as high:low (R6:R7)
; Hint: This is the foundation of multi-byte arithmetic

section .code

; First number: 0x01FF (R6:R7)
loadi   r6, 0x01    ; Load 0x01 into R6 (high byte)
loadi   r7, 0xff    ; Load 0xFF into R7 (low byte)

; Second number: 0x0002 (R0:R1)
loadi   r0, 0x00    ; Load 0x00 into R0 (high byte)
loadi   r1, 0x02    ; Load 0x02 into R1 (low byte)

; Add low bytes first
add     r7, r1      ; ADD R7, R1 (R7 = 0xFF + 0x02 = 0x01, sets C=1 because overflow)

; Add high bytes with carry
adc     r6, r0      ; ADC R6, R0 (R6 = 0x01 + 0x00 + C = 0x02)

; Result: R6:R7 = 0x0201
halt                ; Halt
