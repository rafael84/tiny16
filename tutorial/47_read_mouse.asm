; Exercise 47: Read Mouse Position
; Level: 7 - MMIO & Hardware
;
; Goal: Read mouse X, Y coordinates and button state from MMIO
;       Mouse registers: X at 0xBF02, Y at 0xBF03, Buttons at 0xBF04
;
; Instructions to use: LOADI, LOAD, AND, HALT
; Expected result: R0 = X (0-127), R1 = Y (0-127), R2 = left button state
;
; Hint: Mouse coordinates are 0-127 (fits in 8 bits)
; Hint: Button bits: 0=left, 1=right, 2=middle
; Hint: Use same register pair, just change low byte for each register

section .code

; TODO: Load 0xBF into R6 (MMIO base high)
; TODO: Load 0x02 into R7 (MOUSE_X address)
; TODO: Load from [R6:R7] into R0 (read X coordinate)
; TODO: Load 0x03 into R7 (MOUSE_Y address)
; TODO: Load from [R6:R7] into R1 (read Y coordinate)
; TODO: Load 0x04 into R7 (MOUSE_BUTTONS address)
; TODO: Load from [R6:R7] into R2 (read button state)
; TODO: Load 0x01 into R3 (mask for left button, bit 0)
; TODO: AND R2 with R3 (isolate left button)
; TODO: Halt (R0=X, R1=Y, R2=left button state)
