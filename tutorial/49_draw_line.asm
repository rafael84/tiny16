; Exercise 49: Draw Horizontal Line
; Level: 7 - MMIO & Hardware
;
; Goal: Draw a 10-pixel white horizontal line at (30, 40)
;       Use a loop with post-increment addressing
;
; Instructions to use: LOADI, STORE with [PAIR]+, DEC, JNZ, HALT
; Expected result: 10-pixel white line starting at (30, 40)
;
; Hint: Calculate start address: 0xC000 + (40 << 7) + 30
; Hint: 40 << 7 = 0x1400, + 30 = 0x141E
; Hint: Start: 0xC000 + 0x141E = 0xD41E
; Hint: Use post-increment [R6:R7]+ to draw each pixel

section .code

; Calculate starting address for pixel (30, 40)
; y=40: 40 * 128 = 0x1400, x=30: 0x1E
; Address = 0xC000 + 0x1400 + 0x1E = 0xD41E

; TODO: Load 0xD4 into R6 (start address high)
; TODO: Load 0x1E into R7 (start address low)
; TODO: Load 0xFF into R0 (white color)
; TODO: Load 10 into R1 (pixel count)
loop:
    ; TODO: Store R0 to [R6:R7]+ (draw pixel, advance address)
    ; TODO: Decrement R1
    ; TODO: Jump to loop if R1 != 0
; TODO: Halt
