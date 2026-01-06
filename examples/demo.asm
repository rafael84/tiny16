START:
    ; 1. Read base color offset from low timer bits
    LOADI R6, 0xBF
    LOADI R7, 0x20
    LOAD  R0          ; R0 = Timer value (base color)

    ; 2. Initialize Framebuffer Pointer
    LOADI R6, 0xC0    ; Framebuffer High (0xC000)
    LOADI R7, 0x00    ; Framebuffer Low

DRAW_LOOP:
    ; --- Generate Pixel Color using SHR ---
    ; We take the lower 8 bits of the address (R7),
    ; which corresponds roughly to the X position.
    MOV   R1, R7
    SHR   R1          ; R1 = X / 2
    SHR   R1          ; R1 = X / 4
    ADD   R1, R0      ; Add timer offset for animation

    ; Write to screen
    STORE R1

    ; 3. Increment Pointer
    INC   R7
    JNZ   DRAW_LOOP   ; If R7 didn't wrap to 0, keep filling
    INC   R6          ; If R7 wrapped, increment High byte (R6)

    ; Check if we finished the 16KB framebuffer (reaches 0xFF+1)
    LOADI R3, 0xFF
    MOV   R4, R3
    SUB   R4, R6      ; Compare R6 to 0xFF
    JNZ   DRAW_LOOP   ; If R6 < 0xFF, keep drawing

    ; 4. Signal frame complete (VSYNC swap)
    LOADI R6, 0xBF
    LOADI R7, 0x23
    LOADI R1, 1
    STORE R1

    ; 5. Wait for VSync
WAIT_FRAME:
    LOADI R7, 0x22    ; FRAME_COUNT
    LOAD  R1
    SUB   R1, R2      ; R2 holds last frame count
    JZ    WAIT_FRAME
    MOV   R2, R1      ; Update last frame count

    JMP   START       ; Repeat
