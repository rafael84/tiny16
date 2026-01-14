; Exercise 17: Draw Horizontal Line
; Goal: Draw a horizontal line of 10 white pixels starting at (20, 30)
; Framebuffer at 0xC000, pixels are consecutive in memory for same row
; Instructions to use: LOADI, STORE, INC, DEC, JNZ
; Expected result: Horizontal line from (20, 30) to (29, 30)

section .code

; TODO: Your code here
; Step 1: Calculate start address = 0xC000 + (30 << 7) + 20
;         = 0xC000 + 3840 + 20 = 0xCF14
; Step 2: Set R6:R7 to start address
; Step 3: Load color (0xFF = white) into R0
; Step 4: Load counter (10) into R1
; Step 5: Loop:
;         - STORE R0, [R6:R7] (draw pixel)
;         - INC R7 (move to next pixel, same row)
;         - DEC R1 (decrement counter)
;         - JNZ loop
; Note: Use -df flag to see framebuffer

HALT
