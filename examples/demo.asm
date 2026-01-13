; Bouncing Smiley Demo - Animated sprite with collision detection (with subroutines)
;
; This demo demonstrates:
; - Using .code and .data sections
; - CALL/RET for subroutines
; - Defining sprite data with DB directive
; - Reading from data section (0x4000)
; - Writing to framebuffer (0xC000)
; - RGB332 color format (0xFC = yellow, 0x03 = dark blue)
; - Address calculation for 128x128 display
; - Animation with velocity and boundary checking
; - Stack operations (PUSH/POP) for register preservation
;
; Memory layout:
; 0x4000: sprite data (64 bytes)
; 0x4040: sprite_x (1 byte)
; 0x4041: sprite_y (1 byte)
; 0x4042: vel_x (1 byte, signed: 0=negative, 1=positive)
; 0x4043: vel_y (1 byte, signed: 0=negative, 1=positive)
; 0x4044: initialized (1 byte, 0xAA when initialized)

section .code

START:
    ; Initialize sprite position and velocity (first time only)
    LOADI R6, 0x40
    LOADI R7, 0x44
    LOAD  R0          ; Check init flag
    LOADI R1, 0xAA
    SUB   R0, R1      ; Compare with 0xAA
    JZ    SKIP_INIT   ; Skip if already initialized

    CALL  INIT_SPRITE

SKIP_INIT:
    CALL  CLEAR_SCREEN
    CALL  DRAW_SPRITE_ROUTINE
    CALL  UPDATE_POSITION
    CALL  WAIT_FRAME_ROUTINE
    JMP   SKIP_INIT   ; Loop forever

; ============================================================================
; INIT_SPRITE - Initialize sprite position and velocity
; Inputs: None
; Outputs: None
; Modifies: R0, R6, R7
; ============================================================================
INIT_SPRITE:
    ; Set initial position (120, 25) - off-center to avoid diagonal-only bouncing
    LOADI R0, 120
    LOADI R6, 0x40
    LOADI R7, 0x40
    STORE R0          ; sprite_x = 120
    LOADI R0, 25
    LOADI R7, 0x41
    STORE R0          ; sprite_y = 25

    ; Set initial velocity (moving down-right)
    LOADI R0, 1
    LOADI R7, 0x42
    STORE R0          ; vel_x = 1 (positive)
    LOADI R7, 0x43
    STORE R0          ; vel_y = 1 (positive)

    ; Mark as initialized
    LOADI R0, 0xAA
    LOADI R7, 0x44
    STORE R0          ; initialized = 0xAA
    RET

; ============================================================================
; CLEAR_SCREEN - Clear framebuffer to dark blue
; Inputs: None
; Outputs: None
; Modifies: R0, R1, R6, R7
; ============================================================================
CLEAR_SCREEN:
    LOADI R0, 0x03    ; Dark blue color
    LOADI R6, 0xC0    ; Framebuffer High (0xC000)
    LOADI R7, 0x00    ; Framebuffer Low

CLEAR_LOOP:
    STORE R0
    INC   R7
    JNZ   CLEAR_LOOP
    INC   R6          ; Increment high byte when low byte wraps
    ; Check if we've wrapped back to 0x00 (cleared full framebuffer 0xC000-0xFFFF)
    LOADI R1, 0
    CMP   R6, R1      ; Sets Z if R6 == 0
    JNZ   CLEAR_LOOP  ; Continue if R6 != 0
    RET

; ============================================================================
; DRAW_SPRITE_ROUTINE - Draw 8x8 sprite at current position
; Inputs: None (reads sprite_x, sprite_y from memory)
; Outputs: None
; Modifies: R0, R1, R2, R3, R4, R5, R6, R7
; ============================================================================
DRAW_SPRITE_ROUTINE:
    ; Load sprite position from memory into R1, R2
    LOADI R6, 0x40
    LOADI R7, 0x41
    LOAD  R1          ; R1 = sprite_y (current Y)
    LOADI R7, 0x40
    LOAD  R2          ; R2 = sprite_x (will be reset each row)

    XOR   R0, R0      ; Sprite byte counter (0-63)

DRAW_SPRITE_LOOP:
    ; Reload start_x from memory for each row
    LOADI R6, 0x40
    LOADI R7, 0x40
    LOAD  R2          ; R2 = start_x
    LOADI R3, 8       ; 8 pixels per row

DRAW_SPRITE_ROW:
    ; Read pixel from sprite data (0x4000 + offset)
    LOADI R6, 0x40    ; Data section at 0x4000
    MOV   R7, R0
    LOAD  R4          ; R4 = pixel color

    ; Calculate framebuffer address: 0xC000 + (Y << 7) + X
    ; Since Y * 128 can exceed 255, we need to calculate the high and low bytes
    ; Y=60, X=60: offset = 60*128 + 60 = 7680 + 60 = 7740 = 0x1E3C

    ; Save current state
    PUSH  R0
    PUSH  R1
    PUSH  R2
    PUSH  R3
    PUSH  R4

    ; Calculate framebuffer address using R5, R6, R7 as temps
    ; R5 = Y >> 1 (for high byte)
    MOV   R5, R1
    SHR   R5          ; R5 = Y / 2

    ; R6 = low byte of (Y * 128)
    MOV   R6, R1
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6
    SHL   R6          ; R6 = (Y << 7) & 0xFF

    ; Add X to low byte
    ADD   R6, R2      ; R6 = low byte of offset

    ; Set framebuffer pointer
    MOV   R7, R6      ; R7 = low byte
    LOADI R6, 0xC0
    ADD   R6, R5      ; R6 = 0xC0 + (Y >> 1)

    ; Restore and write pixel
    POP   R4
    STORE R4          ; Write to framebuffer

    ; Restore registers
    POP   R3
    POP   R2
    POP   R1
    POP   R0

    ; Move to next pixel
    INC   R0          ; Next sprite byte
    INC   R2          ; Next X position
    DEC   R3          ; Decrement column counter
    JNZ   DRAW_SPRITE_ROW

    ; Move to next row
    INC   R1          ; Next Y position

    ; Check if done (64 bytes = 8 rows * 8 cols)
    LOADI R3, 64
    CMP   R0, R3      ; Sets Z if R0 == 64
    JNZ   DRAW_SPRITE_LOOP
    RET

; ============================================================================
; UPDATE_POSITION - Update sprite position with collision detection
; Inputs: None (reads sprite_x, sprite_y, vel_x, vel_y from memory)
; Outputs: None (writes back updated values)
; Modifies: R0, R1, R2, R3, R4, R5, R6, R7
; ============================================================================
UPDATE_POSITION:
    ; Load current position and velocities
    LOADI R6, 0x40    ; Data section high byte
    LOADI R7, 0x40
    LOAD  R0          ; R0 = sprite_x
    LOADI R7, 0x41
    LOAD  R1          ; R1 = sprite_y
    LOADI R7, 0x42
    LOAD  R2          ; R2 = vel_x (1=positive, 0=negative)
    LOADI R7, 0x43
    LOAD  R3          ; R3 = vel_y

    ; Update X position (R2 = vel_x: 1=right, 0=left)
    LOADI R5, 0
    CMP   R2, R5      ; Sets Z if R2 == 0
    JZ    MOVE_X_LEFT

    ; Moving right - check if already at right edge
    MOV   R4, R0
    LOADI R5, 120
    SUB   R4, R5      ; Check if X >= 120
    JZ    BOUNCE_X
    INC   R0          ; Move right
    JMP   UPDATE_Y

MOVE_X_LEFT:
    ; Moving left - check if already at left edge
    LOADI R5, 0
    CMP   R0, R5      ; Check if X == 0
    JZ    BOUNCE_X
    DEC   R0          ; Move left
    JMP   UPDATE_Y

BOUNCE_X:
    ; Reverse X velocity (toggle between 0 and 1)
    LOADI R4, 1
    SUB   R4, R2
    MOV   R2, R4
    LOADI R6, 0x40
    LOADI R7, 0x42
    STORE R2          ; Save new vel_x
    JMP   UPDATE_Y    ; Continue to Y update

UPDATE_Y:
    ; Update Y position (R3 = vel_y: 1=down, 0=up)
    LOADI R5, 0
    CMP   R3, R5      ; Sets Z if R3 == 0
    JZ    MOVE_Y_UP

    ; Moving down - check if already at bottom edge
    MOV   R4, R1
    LOADI R5, 120
    SUB   R4, R5      ; Check if Y >= 120
    JZ    BOUNCE_Y
    INC   R1          ; Move down
    JMP   SAVE_POS

MOVE_Y_UP:
    ; Moving up - check if already at top edge
    LOADI R5, 0
    CMP   R1, R5      ; Check if Y == 0
    JZ    BOUNCE_Y
    DEC   R1          ; Move up
    JMP   SAVE_POS

BOUNCE_Y:
    ; Reverse Y velocity (toggle between 0 and 1)
    LOADI R4, 1
    SUB   R4, R3
    MOV   R3, R4
    LOADI R6, 0x40
    LOADI R7, 0x43
    STORE R3          ; Save new vel_y

SAVE_POS:
    ; Save new position
    LOADI R6, 0x40
    LOADI R7, 0x40
    STORE R0          ; Save sprite_x
    LOADI R7, 0x41
    STORE R1          ; Save sprite_y
    RET

; ============================================================================
; WAIT_FRAME_ROUTINE - Signal frame complete and wait for next frame
; Inputs: None
; Outputs: None
; Modifies: R0, R1, R6, R7
; ============================================================================
WAIT_FRAME_ROUTINE:
    ; Signal frame complete (VSYNC)
    LOADI R6, 0xBF
    LOADI R7, 0x23
    LOADI R0, 1
    STORE R0

    ; Wait for next frame
WAIT_FRAME_LOOP:
    LOADI R6, 0xBF
    LOADI R7, 0x22    ; FRAME_COUNT
    LOAD  R0
    SUB   R0, R1      ; Compare with last frame count
    JZ    WAIT_FRAME_LOOP  ; Wait until frame changes
    MOV   R1, R0      ; Update last frame count
    RET

section .data

; 8x8 sprite data (smiley face in RGB332 format)
; RGB332: RRRGGGBB
; 0x00 = black (00000000)
; 0xFC = yellow (11111100 = full red + full green)
sprite_data:
    DB 0x03, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0x03
    DB 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC
    DB 0xFC, 0x00, 0xFC, 0xFC, 0xFC, 0xFC, 0x00, 0xFC
    DB 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC
    DB 0xFC, 0x00, 0xFC, 0xFC, 0xFC, 0xFC, 0x00, 0xFC
    DB 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC
    DB 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC
    DB 0x03, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0x03

; Animation state (at 0x4040)
sprite_x:      DB 0     ; X position
sprite_y:      DB 0     ; Y position
vel_x:         DB 0     ; X velocity (1=right, 0=left)
vel_y:         DB 0     ; Y velocity (1=down, 0=up)
initialized:   DB 0     ; Initialization flag (0xAA when initialized)
