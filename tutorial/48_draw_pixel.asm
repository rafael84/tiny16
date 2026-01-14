; Exercise 48: Draw Single Pixel
; Level: 7 - MMIO & Hardware
;
; Goal: Draw a white pixel at coordinates (64, 64) - center of screen
;       Framebuffer address = 0xC000 + (y << 7) + x
;
; Instructions to use: LOADI, STORE, HALT
; Expected result: White pixel at screen center
;
; Hint: Screen is 128x128 pixels, 1 byte per pixel (RGB332)
; Hint: Address calculation: 0xC000 + (y * 128) + x
; Hint: y * 128 = y << 7 (shift left 7 times)
; Hint: RGB332: RRR GGG BB (3 red, 3 green, 2 blue bits)
; Hint: 0xFF = white, 0x00 = black, 0xE0 = red, 0x1C = green, 0x03 = blue

section .code

; Calculate address for pixel (64, 64)
; y=64: 64 << 7 = 0x2000, x=64: 0x40
; Address = 0xC000 + 0x2000 + 0x40 = 0xE040

; TODO: Load 0xE0 into R6 (address high byte)
; TODO: Load 0x40 into R7 (address low byte)
; TODO: Load 0xFF into R0 (white color)
; TODO: Store R0 to [R6:R7] (draw pixel)
; TODO: Halt
