; Exercise 15: Read Mouse Position
; Goal: Read mouse X and Y coordinates from MMIO
; MOUSE_X at 0xBF02, MOUSE_Y at 0xBF03
; Instructions to use: LOADI, LOAD
; Expected result: R0 = mouse X (0-127), R1 = mouse Y (0-127)

section .code

; TODO: Your code here
; Step 1: Set R6:R7 to 0xBF02 (MOUSE_X address)
; Step 2: LOAD mouse X into R0
; Step 3: Change R7 to 0x03 (MOUSE_Y address)
; Step 4: LOAD mouse Y into R1

HALT
