; ADC and SBC demonstration
; Shows multi-byte arithmetic using Add with Carry and Subtract with Carry

section .code

; 16-bit addition using ADD + ADC
; Add 0x01FF (511) + 0x0002 (2) = 0x0201 (513)

    LOADI R6, 0x01      ; R6:R7 = 0x01FF (first 16-bit number)
    LOADI R7, 0xFF
    LOADI R0, 0x00      ; R0:R1 = 0x0002 (second 16-bit number)
    LOADI R1, 0x02

    ADD   R7, R1        ; Add low bytes: 0xFF + 0x02 = 0x101
                        ; R7 = 0x01, C = 1 (overflow)
    ADC   R6, R0        ; Add high bytes + carry: 0x01 + 0x00 + 1 = 0x02
                        ; R6 = 0x02, C = 0

; Result: R6:R7 = 0x0201 (513) ✓

; 16-bit subtraction using SUB + SBC
; Subtract 0x0201 (513) - 0x00FF (255) = 0x0102 (258)

    LOADI R2, 0x00      ; R2:R3 = 0x00FF (value to subtract)
    LOADI R3, 0xFF

    SUB   R7, R3        ; Subtract low bytes: 0x01 - 0xFF = 0x02
                        ; R7 = 0x02, C = 1 (borrow occurred)
    SBC   R6, R2        ; Subtract high bytes - borrow: 0x02 - 0x00 - 1 = 0x01
                        ; R6 = 0x01, C = 0

; Result: R6:R7 = 0x0102 (258) ✓

    HALT

; Expected final state:
; R6 = 0x01
; R7 = 0x02
; R6:R7 = 0x0102 (258)
