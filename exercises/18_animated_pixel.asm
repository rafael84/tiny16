; Exercise 18: Animated Pixel (Advanced)
; Goal: Draw a pixel that moves based on keyboard input
; Use KEYS_STATE to read input, update position, draw pixel
; Instructions to use: LOADI, LOAD, STORE, AND, JZ, JMP
; Expected result: Pixel moves with arrow keys
; Note: This is a continuous program, will run until halted

section .code

; TODO: Your code here
; This is a challenging exercise combining input and output!
; Algorithm:
; 1. Initialize position (x=64, y=64 in center)
; 2. Main loop:
;    a. Read KEYS_STATE
;    b. Check each direction bit
;    c. Update x/y coordinates
;    d. Calculate framebuffer address
;    e. Draw pixel
;    f. Repeat
;
; Hints:
; - Store x in R0, y in R1
; - KEYS_STATE bits: 7=Down, 6=Up, 5=Left, 4=Right
; - Framebuffer addr = 0xC000 + (y << 7) + x
; - Use multiple loops for each direction check

HALT
