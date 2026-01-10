; Exercise 16: Draw a Pixel
; Goal: Draw a single white pixel at coordinates (10, 10) on framebuffer
; Framebuffer at 0xC000, address = 0xC000 + (y << 7) + x
; Color: RGB332 format, 0xFF = white
; Instructions to use: LOADI, STORE
; Expected result: White pixel at (10, 10)

section .code

; TODO: Your code here
; Step 1: Calculate address = 0xC000 + (10 << 7) + 10
;         = 0xC000 + 1280 + 10 = 0xC000 + 1290 = 0xC50A
; Step 2: Set R6:R7 to 0xC50A
; Step 3: Load 0xFF (white) into R0
; Step 4: STORE R0 to draw the pixel
; Note: Use -df flag to see framebuffer in dump

HALT
