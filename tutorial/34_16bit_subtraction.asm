; Exercise 34: 16-bit Subtraction
; Level: 5 - Advanced Arithmetic
;
; Goal: Subtract two 16-bit numbers using SBC (subtract with carry/borrow)
;       0x0201 - 0x00FF = 0x0102 (513 - 255 = 258)
;
; Instructions to use: LOADI, SUB, SBC, HALT
; Expected result: R6:R7 = 0x0102 (R6 = 0x01, R7 = 0x02)
;
; Hint: SBC R1, R2 means R1 = R1 - R2 - C (subtract with borrow)
; Hint: First subtract low bytes (sets C if borrow), then SBC high bytes
; Hint: C flag = 1 means borrow occurred

section .code

; First number: 0x0201 (R6:R7)
; TODO: Load 0x02 into R6 (high byte)
; TODO: Load 0x01 into R7 (low byte)

; Second number: 0x00FF (R0:R1)
; TODO: Load 0x00 into R0 (high byte)
; TODO: Load 0xFF into R1 (low byte)

; Subtract low bytes first
; TODO: SUB R7, R1 (R7 = 0x01 - 0xFF, sets C=1 because borrow)

; Subtract high bytes with borrow
; TODO: SBC R6, R0 (R6 = 0x02 - 0x00 - C = 0x01)

; Result: R6:R7 = 0x0102
; TODO: Halt
