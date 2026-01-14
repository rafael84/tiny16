; Tutorial 45: Read Input State
; Level: 7 - MMIO & Hardware
;
; Goal: Read keyboard/gamepad state from MMIO register and test specific key
;       Check if the "Up" key (bit 6) is pressed
;
; Instructions to use: LOADI, LOAD, AND, JZ, HALT
; Expected result: R0 = 1 if Up pressed, 0 if not
;
; Hint: KEYS_STATE at 0xBF00 contains current button states
; Hint: Bit layout: 7=Down, 6=Up, 5=Left, 4=Right, 3=B, 2=A, 1=Start, 0=Select
; Hint: Use AND to isolate specific bit
; Hint: This is memory-mapped I/O - reading special address gets hardware state

section .code

; TODO: Load 0xBF into R6 (MMIO base address high)
; TODO: Load 0x00 into R7 (KEYS_STATE address low)
; TODO: Load from [R6:R7] into R0 (read key state)
; TODO: Load 0x40 into R1 (mask for Up key, bit 6)
; TODO: AND R0 with R1 (isolate Up bit)
; TODO: If result is 0 (Z=1), jump to not_pressed
; TODO: Load 1 into R0 (Up is pressed)
; TODO: Jump to done
not_pressed:
    ; TODO: Load 0 into R0 (Up is not pressed)
done:
    ; TODO: Halt
