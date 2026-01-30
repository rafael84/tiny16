; =============================================================================
; APU Demo - Demonstrates the tiny16 Audio Processing Unit
; =============================================================================
; Controls:
;   A: Play simple melody (with harmony and bass)
;   B: Play explosion/boom sound effect
;   Up: Play coin/pickup sound effect
;   Down: Play jump sound effect
;   Select: Toggle pulse channel 0 (CH0)
;   Start: Toggle pulse channel 1 (CH1)
;   Left: Toggle triangle channel (TRI)
;   Right: Toggle noise channel (NOISE)
; =============================================================================

.include "../stdlib/tiny16.inc"

section .code

; =============================================================================
; Main entry point
; =============================================================================
main:
    APU_INIT 8              ; Initialize APU with master volume 8
    CALL init_text_display  ; Initialize text display

; =============================================================================
; Main loop
; =============================================================================
main_loop:
    CALL wait_frame
    CALL handle_input
    CALL render_frame
    JMP main_loop

; =============================================================================
; Input handling
; =============================================================================
handle_input:
    ; Read pressed keys (edge detection)
    SETADDR KEYS_PRESSED_ADDR
    LOAD R0, [R6:R7]

    ; A button - play melody
    MOV R1, R0
    LOADI R2, KEY_A
    AND R1, R2
    JNZ do_play_melody

    ; B button - play explosion
    MOV R1, R0
    LOADI R2, KEY_B
    AND R1, R2
    JNZ do_play_explosion

    ; Up button - play coin sound
    MOV R1, R0
    LOADI R2, KEY_UP
    AND R1, R2
    JNZ do_play_coin

    ; Down button - play jump sound
    MOV R1, R0
    LOADI R2, KEY_DOWN
    AND R1, R2
    JNZ do_play_jump

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

do_play_melody:
    CALL play_melody
    RET

do_play_explosion:
    CALL play_explosion
    RET

do_play_coin:
    CALL play_coin
    RET

do_play_jump:
    CALL play_jump
    RET

; =============================================================================
; Play a simple melody with harmony and bass
; Gentle excerpt inspired by Satie's Gymnopedie No.1 (approx, with sharps)
; =============================================================================
play_melody:
    PUSH R0

    ; Soft ADSR for a gentle melody
    APU_CH0_ENV 2, 4, 10, 6
    APU_CH1_ENV 2, 4, 10, 6
    APU_WAVE_ENV 2, 4, 12, 6

    ; Load a simple sine-like wave into wave RAM
    APU_WAVE_WRITE 0, 8
    APU_WAVE_WRITE 1, 9
    APU_WAVE_WRITE 2, 10
    APU_WAVE_WRITE 3, 11
    APU_WAVE_WRITE 4, 12
    APU_WAVE_WRITE 5, 13
    APU_WAVE_WRITE 6, 14
    APU_WAVE_WRITE 7, 15
    APU_WAVE_WRITE 8, 14
    APU_WAVE_WRITE 9, 13
    APU_WAVE_WRITE 10, 12
    APU_WAVE_WRITE 11, 11
    APU_WAVE_WRITE 12, 10
    APU_WAVE_WRITE 13, 9
    APU_WAVE_WRITE 14, 8
    APU_WAVE_WRITE 15, 7
    APU_WAVE_WRITE 16, 6
    APU_WAVE_WRITE 17, 5
    APU_WAVE_WRITE 18, 4
    APU_WAVE_WRITE 19, 3
    APU_WAVE_WRITE 20, 2
    APU_WAVE_WRITE 21, 1
    APU_WAVE_WRITE 22, 0
    APU_WAVE_WRITE 23, 1
    APU_WAVE_WRITE 24, 2
    APU_WAVE_WRITE 25, 3
    APU_WAVE_WRITE 26, 4
    APU_WAVE_WRITE 27, 5
    APU_WAVE_WRITE 28, 6
    APU_WAVE_WRITE 29, 7
    APU_WAVE_WRITE 30, 8
    APU_WAVE_WRITE 31, 8

    ; Phrase 1: D5 melody, F#4 harmony, D3 bass
    APU_WAVE_LEN 22
    APU_CH0_NOTE NOTE_D5, 10, APU_DUTY_25
    APU_CH1_NOTE NOTE_FS4, 8, APU_DUTY_12_5
    APU_WAVE_NOTE NOTE_D3, 6
    LOADI R0, 22
    CALL wait_frames

    ; Phrase 2: E5 melody, G4 harmony, E3 bass
    APU_WAVE_LEN 22
    APU_CH0_NOTE NOTE_E5, 10, APU_DUTY_25
    APU_CH1_NOTE NOTE_G4, 8, APU_DUTY_12_5
    APU_WAVE_NOTE NOTE_E3, 6
    LOADI R0, 22
    CALL wait_frames

    ; Phrase 3: F#5 melody, A4 harmony, F#3 bass
    APU_WAVE_LEN 22
    APU_CH0_NOTE NOTE_FS5, 10, APU_DUTY_25
    APU_CH1_NOTE NOTE_A4, 8, APU_DUTY_12_5
    APU_WAVE_NOTE NOTE_FS3, 6
    LOADI R0, 22
    CALL wait_frames

    ; Phrase 4: A5 melody, C#4 harmony, A3 bass
    APU_WAVE_LEN 22
    APU_CH0_NOTE NOTE_A5, 10, APU_DUTY_25
    APU_CH1_NOTE NOTE_CS4, 8, APU_DUTY_12_5
    APU_WAVE_NOTE NOTE_A3, 6
    LOADI R0, 22
    CALL wait_frames

    ; Phrase 5: F#5 melody, A4 harmony, F#3 bass
    APU_WAVE_LEN 22
    APU_CH0_NOTE NOTE_FS5, 10, APU_DUTY_25
    APU_CH1_NOTE NOTE_A4, 8, APU_DUTY_12_5
    APU_WAVE_NOTE NOTE_FS3, 6
    LOADI R0, 22
    CALL wait_frames

    ; Phrase 6: E5 melody, G4 harmony, E3 bass
    APU_WAVE_LEN 22
    APU_CH0_NOTE NOTE_E5, 10, APU_DUTY_25
    APU_CH1_NOTE NOTE_G4, 8, APU_DUTY_12_5
    APU_WAVE_NOTE NOTE_E3, 6
    LOADI R0, 22
    CALL wait_frames

    ; Phrase 7: C#5 melody, E4 harmony, A3 bass
    APU_WAVE_LEN 22
    APU_CH0_NOTE NOTE_CS5, 10, APU_DUTY_25
    APU_CH1_NOTE NOTE_E4, 8, APU_DUTY_12_5
    APU_WAVE_NOTE NOTE_A3, 6
    LOADI R0, 22
    CALL wait_frames

    ; Phrase 8: D5 melody (hold), F#4 harmony, D3 bass
    APU_WAVE_LEN 36
    APU_CH0_NOTE NOTE_D5, 10, APU_DUTY_25
    APU_CH1_NOTE NOTE_FS4, 8, APU_DUTY_12_5
    APU_WAVE_NOTE NOTE_D3, 6
    LOADI R0, 36
    CALL wait_frames

    ; Silence all channels
    APU_ALL_OFF

    POP R0
    RET

; =============================================================================
; Play explosion sound effect
; Uses noise channel with volume fade
; =============================================================================
play_explosion:
    PUSH R1

    ; Start noise: mid period, max volume, long mode
    ; Envelope handles the fade
    APU_CH3_ENV 1, 6, 0, 6
    APU_NOISE 8, 15, 0

    ; Let the envelope tail out
    LOADI R1, 18
    CALL wait_frames
    APU_NOISE_OFF

    POP R1
    RET

; =============================================================================
; Play coin/pickup sound effect
; Quick ascending arpeggio on pulse channel
; =============================================================================
play_coin:
    PUSH R0

    ; Note 1: B4
    APU_CH0_ENV 1, 2, 8, 3
    APU_CH0_LEN 4
    APU_CH0_NOTE NOTE_B4, 12, APU_DUTY_25
    LOADI R0, 3
    CALL wait_frames

    ; Note 2: E5
    APU_CH0_LEN 6
    APU_CH0_NOTE NOTE_E5, 12, APU_DUTY_25
    LOADI R0, 6
    CALL wait_frames

    ; Silence
    APU_CH0_OFF

    POP R0
    RET

; =============================================================================
; Play jump sound effect
; Descending pitch sweep on pulse channel
; =============================================================================
play_jump:
    PUSH R0

    ; Start at high pitch (C5)
    APU_CH0_ENV 1, 4, 6, 6
    APU_CH0_SWEEP 4, 1, 2
    APU_CH0_LEN 12
    APU_CH0_NOTE NOTE_C5, 10, APU_DUTY_12_5

    ; Let sweep run
    LOADI R0, 12
    CALL wait_frames

    ; Silence
    APU_CH0_OFF

    POP R0
    RET

; =============================================================================
; Utility: Wait for one frame
; =============================================================================
wait_frame:
    PUSH R0
    PUSH R1
    PUSH R6
    PUSH R7
    SETADDR FRAME_COUNT_ADDR
    LOAD R0, [R6:R7]
wf_loop:
    LOAD R1, [R6:R7]
    CMP R0, R1
    JZ wf_loop
    POP R7
    POP R6
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

; =============================================================================
; Utility: Render frame (trigger PPU with sprites enabled)
; =============================================================================
render_frame:
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    LOADI R0, 0x82        ; Sprites (0x02) + Render Now (0x80)
    STORE R0, [R6:R7]
    RET

; =============================================================================
; Text Display Functions
; =============================================================================

; init_text_display - Initialize text display with instructions
init_text_display:
    PUSH R0
    PUSH R1
    PUSH R2
    PUSH R3

    ; Hide all 64 OAM sprites first
    OAM_HIDE_ALL

    ; Sprite counter starts at 0
    LOADI R3, 0

    ; Display line 1: "A MELODY" at (2, 2)
    LOADI R0, 2
    LOADI R1, 2
    SETADDR text_line1
    CALL draw_text_line

    ; Display line 2: "B EXPLOSION" at (2, 10)
    LOADI R0, 2
    LOADI R1, 10
    SETADDR text_line2
    CALL draw_text_line

    ; Display line 3: "UP COIN" at (2, 18)
    LOADI R0, 2
    LOADI R1, 18
    SETADDR text_line3
    CALL draw_text_line

    ; Display line 4: "DOWN JUMP" at (2, 26)
    LOADI R0, 2
    LOADI R1, 26
    SETADDR text_line4
    CALL draw_text_line

    ; Display line 5: "SELECT CH0" at (2, 38)
    LOADI R0, 2
    LOADI R1, 38
    SETADDR text_line5
    CALL draw_text_line

    ; Display line 6: "START CH1" at (2, 46)
    LOADI R0, 2
    LOADI R1, 46
    SETADDR text_line6
    CALL draw_text_line

    ; Display line 7: "LEFT CH2" at (2, 54)
    LOADI R0, 2
    LOADI R1, 54
    SETADDR text_line7
    CALL draw_text_line

    ; Display line 8: "RIGHT CH3" at (2, 62)
    LOADI R0, 2
    LOADI R1, 62
    SETADDR text_line8
    CALL draw_text_line

    ; Render the sprites we just wrote
    CALL render_frame

    POP R3
    POP R2
    POP R1
    POP R0
    RET

; Include text rendering function from stdlib
.include "../stdlib/text.inc"

; =============================================================================
; Data Section
; =============================================================================
section .data

text_line1: DB "A MELODY", 0
text_line2: DB "B BOOM", 0
text_line3: DB "UP COIN", 0
text_line4: DB "DN JUMP", 0
text_line5: DB "SEL CH0", 0
text_line6: DB "STA CH1", 0
text_line7: DB "LT TRI", 0
text_line8: DB "RT NOISE", 0

.include "font.inc"
