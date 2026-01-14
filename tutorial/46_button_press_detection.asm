; Exercise 46: Button Press Detection (Edge Detection)
; Level: 7 - MMIO & Hardware
;
; Goal: Detect button press using KEYS_PRESSED register
;       This register gives edge detection - triggers once per press
;
; Instructions to use: LOADI, LOAD, AND, JZ, HALT
; Expected result: R0 = 1 if A button just pressed, 0 if not
;
; Hint: KEYS_PRESSED at 0xBF01 is cleared after reading
; Hint: Use this for actions that should trigger once per press (jump, shoot)
; Hint: KEYS_STATE is for continuous actions (move while held)
; Hint: Bit 2 = A button

section .code

; TODO: Load 0xBF into R6 (MMIO base high)
; TODO: Load 0x01 into R7 (KEYS_PRESSED address)
; TODO: Load from [R6:R7] into R0 (read pressed keys, auto-clears)
; TODO: Load 0x04 into R1 (mask for A button, bit 2)
; TODO: AND R0 with R1 (isolate A button bit)
; TODO: If result is 0 (Z=1), jump to not_pressed
; TODO: Load 1 into R0 (A was just pressed)
; TODO: Jump to done
not_pressed:
    ; TODO: Load 0 into R0 (A not pressed this frame)
done:
    ; TODO: Halt
