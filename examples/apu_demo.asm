; =============================================================================
; APU Demo - Demonstrates the tiny16 Audio Processing Unit
; =============================================================================
; Controls:
;   A: Play C major scale
;   B: Play explosion sound effect
;   Select: Toggle pulse channel 0
;   Start: Toggle pulse channel 1
;   Left: Toggle triangle channel
;   Right: Toggle noise channel
;   Up/Down: Change pitch on channel 0
; =============================================================================

.include "../stdlib/tiny16.inc"

section .code

; =============================================================================
; Main entry point
; =============================================================================
main:
    ; Initialize APU with master volume 12
    APU_INIT 12

    ; Start with channel 0 playing A4
    APU_CH0_NOTE NOTE_A4, 12, APU_DUTY_50

; =============================================================================
; Main loop
; =============================================================================
main_loop:
    CALL handle_input
    CALL wait_frame
    JMP main_loop

; =============================================================================
; Input handling
; =============================================================================
handle_input:
    ; Read pressed keys (edge detection)
    SETADDR KEYS_PRESSED_ADDR
    LOAD R0, [R6:R7]

    ; A button - play scale
    MOV R1, R0
    LOADI R2, KEY_A
    AND R1, R2
    JNZ do_play_scale

    ; B button - play explosion
    MOV R1, R0
    LOADI R2, KEY_B
    AND R1, R2
    JNZ do_play_explosion

    ; Select - toggle channel 0
    MOV R1, R0
    LOADI R2, KEY_SELECT
    AND R1, R2
    JNZ toggle_ch0

    ; Start - toggle channel 1
    MOV R1, R0
    LOADI R2, KEY_START
    AND R1, R2
    JNZ toggle_ch1

    ; Left - toggle channel 2
    MOV R1, R0
    LOADI R2, KEY_LEFT
    AND R1, R2
    JNZ toggle_ch2

    ; Right - toggle channel 3
    MOV R1, R0
    LOADI R2, KEY_RIGHT
    AND R1, R2
    JNZ toggle_ch3

    ; Up - increase frequency
    MOV R1, R0
    LOADI R2, KEY_UP
    AND R1, R2
    JNZ freq_up

    ; Down - decrease frequency
    MOV R1, R0
    LOADI R2, KEY_DOWN
    AND R1, R2
    JNZ freq_down

    RET

toggle_ch0:
    SETADDR APU_CH0_CTRL
    LOAD R0, [R6:R7]
    LOADI R1, 0x01
    XOR R0, R1
    STORE R0, [R6:R7]
    RET

toggle_ch1:
    SETADDR APU_CH1_CTRL
    LOAD R0, [R6:R7]
    LOADI R1, 0x01
    XOR R0, R1
    STORE R0, [R6:R7]
    RET

toggle_ch2:
    SETADDR APU_CH2_CTRL
    LOAD R0, [R6:R7]
    LOADI R1, 0x01
    XOR R0, R1
    STORE R0, [R6:R7]
    RET

toggle_ch3:
    SETADDR APU_CH3_CTRL
    LOAD R0, [R6:R7]
    LOADI R1, 0x01
    XOR R0, R1
    STORE R0, [R6:R7]
    RET

freq_up:
    SETADDR APU_CH0_FREQ_LO
    LOAD R0, [R6:R7]
    LOADI R1, 3
    ADD R0, R1
    STORE R0, [R6:R7]
    RET

freq_down:
    SETADDR APU_CH0_FREQ_LO
    LOAD R0, [R6:R7]
    LOADI R1, 3
    SUB R0, R1
    STORE R0, [R6:R7]
    RET

do_play_scale:
    CALL play_scale
    RET

do_play_explosion:
    CALL play_explosion
    RET

; =============================================================================
; Play a C major scale using the macros
; =============================================================================
play_scale:
    PUSH R0

    ; C4
    APU_CH0_NOTE NOTE_C4, 12, APU_DUTY_50
    LOADI R0, 12
    CALL wait_frames

    ; D4
    APU_CH0_NOTE NOTE_D4, 12, APU_DUTY_50
    LOADI R0, 12
    CALL wait_frames

    ; E4
    APU_CH0_NOTE NOTE_E4, 12, APU_DUTY_50
    LOADI R0, 12
    CALL wait_frames

    ; F4
    APU_CH0_NOTE NOTE_F4, 12, APU_DUTY_50
    LOADI R0, 12
    CALL wait_frames

    ; G4
    APU_CH0_NOTE NOTE_G4, 12, APU_DUTY_50
    LOADI R0, 12
    CALL wait_frames

    ; A4
    APU_CH0_NOTE NOTE_A4, 12, APU_DUTY_50
    LOADI R0, 12
    CALL wait_frames

    ; B4
    APU_CH0_NOTE NOTE_B4, 12, APU_DUTY_50
    LOADI R0, 12
    CALL wait_frames

    ; C5 (hold longer)
    APU_CH0_NOTE NOTE_C5, 12, APU_DUTY_50
    LOADI R0, 24
    CALL wait_frames

    POP R0
    RET

; =============================================================================
; Play explosion sound effect
; =============================================================================
play_explosion:
    PUSH R1

    ; Start noise: mid period, max volume, long mode
    APU_NOISE 8, 15, 0

    ; Fade out over 15 frames
    LOADI R1, 15

fade_loop:
    CALL wait_frame
    DEC R1
    JZ fade_done

    ; Update volume using register
    APU_NOISE_VOL R1
    JMP fade_loop

fade_done:
    APU_NOISE_OFF

    POP R1
    RET

; =============================================================================
; Utility: Wait for one frame
; =============================================================================
wait_frame:
    PUSH R0
    PUSH R1
    SETADDR FRAME_COUNT_ADDR
    LOAD R0, [R6:R7]
wf_loop:
    LOAD R1, [R6:R7]
    CMP R0, R1
    JZ wf_loop
    POP R1
    POP R0
    RET

; =============================================================================
; Utility: Wait for R0 frames
; =============================================================================
wait_frames:
    PUSH R1
    MOV R1, R0
wfs_loop:
    LOADI R0, 0
    CMP R1, R0
    JZ wfs_done
    CALL wait_frame
    DEC R1
    JMP wfs_loop
wfs_done:
    POP R1
    RET
