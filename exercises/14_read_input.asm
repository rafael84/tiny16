; Exercise 14: Read Input State
; Goal: Read keyboard/gamepad state from MMIO and check if Up is pressed
; KEYS_STATE at 0xBF00: bit 6 = Up, bit 7 = Down, bit 5 = Left, bit 4 = Right
; Instructions to use: LOADI, LOAD, AND
; Expected result: R0 = 0x40 if Up pressed, else R0 = 0

section .code

; TODO: Your code here
; Step 1: Set R6:R7 to 0xBF00 (KEYS_STATE address)
; Step 2: LOAD the current key state into R0
; Step 3: Use AND to isolate bit 6 (Up key = 0x40)
; Result: R0 will be 0x40 if Up is pressed, 0 otherwise

HALT
