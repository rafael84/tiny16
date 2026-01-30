; vim: set ft=asm:
; =============================================================================
; TINY16 APU (AUDIO) LIBRARY
; =============================================================================
; Audio Processing Unit constants and macros
; =============================================================================

; -----------------------------------------------------------------------------
; APU MMIO Registers
; -----------------------------------------------------------------------------
APU_CTRL_ADDR       = MMIO_BASE + 0x40  ; Master control: bit 0=enable, bits 4-7=volume
APU_STATUS_ADDR     = MMIO_BASE + 0x41  ; Status: bits 0-4=channel active flags

; Channel 0 - Pulse 1
APU_CH0_FREQ_LO     = MMIO_BASE + 0x42  ; Frequency low byte
APU_CH0_FREQ_HI     = MMIO_BASE + 0x43  ; Frequency high (bits 0-2)
APU_CH0_VOL         = MMIO_BASE + 0x44  ; Volume (bits 0-3) + duty (bits 4-5)
APU_CH0_CTRL        = MMIO_BASE + 0x45  ; Enable (bit 0) + trigger (bit 1)

; Channel 1 - Pulse 2
APU_CH1_FREQ_LO     = MMIO_BASE + 0x46
APU_CH1_FREQ_HI     = MMIO_BASE + 0x47
APU_CH1_VOL         = MMIO_BASE + 0x48
APU_CH1_CTRL        = MMIO_BASE + 0x49

; Channel 2 - Triangle
APU_CH2_FREQ_LO     = MMIO_BASE + 0x4A
APU_CH2_FREQ_HI     = MMIO_BASE + 0x4B
APU_CH2_VOL         = MMIO_BASE + 0x4C
APU_CH2_CTRL        = MMIO_BASE + 0x4D

; Channel 3 - Noise
APU_CH3_PERIOD      = MMIO_BASE + 0x4E  ; Period (0-15)
APU_CH3_VOL         = MMIO_BASE + 0x4F  ; Volume (0-15)
APU_CH3_CTRL        = MMIO_BASE + 0x50  ; Enable + trigger + mode

; Envelopes (AD = attack/decay, SR = sustain/release)
APU_CH0_ENV_AD      = MMIO_BASE + 0x51  ; Attack (hi nibble) / Decay (lo nibble)
APU_CH0_ENV_SR      = MMIO_BASE + 0x52  ; Sustain (hi nibble) / Release (lo nibble)
APU_CH1_ENV_AD      = MMIO_BASE + 0x53
APU_CH1_ENV_SR      = MMIO_BASE + 0x54
APU_CH2_ENV_AD      = MMIO_BASE + 0x55
APU_CH2_ENV_SR      = MMIO_BASE + 0x56
APU_CH3_ENV_AD      = MMIO_BASE + 0x57
APU_CH3_ENV_SR      = MMIO_BASE + 0x58

; Sweep + Length
APU_CH0_SWEEP       = MMIO_BASE + 0x59  ; rate (hi nibble), dir (bit3), shift (bits 0-2)
APU_CH1_SWEEP       = MMIO_BASE + 0x5A
APU_CH0_LEN         = MMIO_BASE + 0x5B  ; Length in frames (60 Hz)
APU_CH1_LEN         = MMIO_BASE + 0x5C
APU_CH2_LEN         = MMIO_BASE + 0x5D
APU_CH3_LEN         = MMIO_BASE + 0x5E

; Wave channel (CH4)
APU_WAVE_FREQ_LO    = MMIO_BASE + 0x60
APU_WAVE_FREQ_HI    = MMIO_BASE + 0x61
APU_WAVE_VOL        = MMIO_BASE + 0x62
APU_WAVE_CTRL       = MMIO_BASE + 0x63
APU_WAVE_LEN        = MMIO_BASE + 0x64
APU_WAVE_ENV_AD     = MMIO_BASE + 0x65
APU_WAVE_ENV_SR     = MMIO_BASE + 0x66
APU_WAVE_RAM        = MMIO_BASE + 0x70  ; 0x70-0x8F

; -----------------------------------------------------------------------------
; Duty Cycle Values (for CH0_VOL/CH1_VOL bits 4-5)
; -----------------------------------------------------------------------------
APU_DUTY_12_5       = 0x00              ; 12.5% duty cycle
APU_DUTY_25         = 0x10              ; 25% duty cycle
APU_DUTY_50         = 0x20              ; 50% duty cycle (default)
APU_DUTY_75         = 0x30              ; 75% duty cycle

; -----------------------------------------------------------------------------
; Note Frequencies (11-bit values)
; Formula: freq_value = 2048 - (44100 / freq_hz)
; -----------------------------------------------------------------------------
NOTE_C3  = 1711    ; 130.81 Hz
NOTE_CS3 = 1730    ; 138.59 Hz
NOTE_D3  = 1748    ; 146.83 Hz
NOTE_DS3 = 1765    ; 155.56 Hz
NOTE_E3  = 1780    ; 164.81 Hz
NOTE_F3  = 1795    ; 174.61 Hz
NOTE_FS3 = 1810    ; 185.00 Hz
NOTE_G3  = 1823    ; 196.00 Hz
NOTE_GS3 = 1836    ; 207.65 Hz
NOTE_A3  = 1848    ; 220.00 Hz
NOTE_AS3 = 1859    ; 233.08 Hz
NOTE_B3  = 1869    ; 246.94 Hz

NOTE_C4  = 1879    ; 261.63 Hz (middle C)
NOTE_CS4 = 1889    ; 277.18 Hz
NOTE_D4  = 1898    ; 293.66 Hz
NOTE_DS4 = 1906    ; 311.13 Hz
NOTE_E4  = 1914    ; 329.63 Hz
NOTE_F4  = 1922    ; 349.23 Hz
NOTE_FS4 = 1929    ; 369.99 Hz
NOTE_G4  = 1935    ; 392.00 Hz
NOTE_GS4 = 1942    ; 415.30 Hz
NOTE_A4  = 1948    ; 440.00 Hz (standard tuning)
NOTE_AS4 = 1953    ; 466.16 Hz
NOTE_B4  = 1959    ; 493.88 Hz

NOTE_C5  = 1964    ; 523.25 Hz
NOTE_CS5 = 1968    ; 554.37 Hz
NOTE_D5  = 1973    ; 587.33 Hz
NOTE_DS5 = 1977    ; 622.25 Hz
NOTE_E5  = 1981    ; 659.25 Hz
NOTE_F5  = 1985    ; 698.46 Hz
NOTE_FS5 = 1988    ; 739.99 Hz
NOTE_G5  = 1992    ; 783.99 Hz
NOTE_GS5 = 1995    ; 830.61 Hz
NOTE_A5  = 1998    ; 880.00 Hz
NOTE_AS5 = 2001    ; 932.33 Hz
NOTE_B5  = 2003    ; 987.77 Hz

; -----------------------------------------------------------------------------
; APU_INIT master_vol - Initialize APU with master volume (0-15)
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_INIT master_vol
    LOADI R6, MMIO_HI
    LOADI R7, APU_CTRL_ADDR & 0xFF
    LOADI R0, (master_vol << 4) | 0x01
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_OFF - Disable APU (silence all audio)
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_CTRL_ADDR & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH0_NOTE note_value, volume, duty - Play note on pulse channel 0
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_CH0_NOTE note_value, volume, duty
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH0_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, note_value >> 8
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, duty | volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH0_OFF - Disable pulse channel 0
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_CH0_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH0_CTRL & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH1_NOTE note_value, volume, duty - Play note on pulse channel 1
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_CH1_NOTE note_value, volume, duty
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH1_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, note_value >> 8
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, duty | volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH1_OFF - Disable pulse channel 1
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_CH1_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH1_CTRL & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH2_NOTE note_value, volume - Play note on triangle channel (bass)
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_CH2_NOTE note_value, volume
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH2_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, note_value >> 8
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH2_OFF - Disable triangle channel
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_CH2_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH2_CTRL & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH0_ENV attack, decay, sustain, release - Set envelope for pulse channel 0
; -----------------------------------------------------------------------------
.macro APU_CH0_ENV attack, decay, sustain, release
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH0_ENV_AD & 0xFF
    LOADI R0, (attack << 4) | (decay & 0x0F)
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (sustain << 4) | (release & 0x0F)
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH1_ENV attack, decay, sustain, release - Set envelope for pulse channel 1
; -----------------------------------------------------------------------------
.macro APU_CH1_ENV attack, decay, sustain, release
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH1_ENV_AD & 0xFF
    LOADI R0, (attack << 4) | (decay & 0x0F)
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (sustain << 4) | (release & 0x0F)
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH2_ENV attack, decay, sustain, release - Set envelope for triangle channel
; -----------------------------------------------------------------------------
.macro APU_CH2_ENV attack, decay, sustain, release
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH2_ENV_AD & 0xFF
    LOADI R0, (attack << 4) | (decay & 0x0F)
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (sustain << 4) | (release & 0x0F)
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH3_ENV attack, decay, sustain, release - Set envelope for noise channel
; -----------------------------------------------------------------------------
.macro APU_CH3_ENV attack, decay, sustain, release
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH3_ENV_AD & 0xFF
    LOADI R0, (attack << 4) | (decay & 0x0F)
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (sustain << 4) | (release & 0x0F)
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH0_SWEEP rate, dir, shift - Set sweep for pulse channel 0
; dir: 0=up, 1=down
; -----------------------------------------------------------------------------
.macro APU_CH0_SWEEP rate, dir, shift
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH0_SWEEP & 0xFF
    LOADI R0, (rate << 4) | ((dir & 0x01) << 3) | (shift & 0x07)
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CH1_SWEEP rate, dir, shift - Set sweep for pulse channel 1
; -----------------------------------------------------------------------------
.macro APU_CH1_SWEEP rate, dir, shift
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH1_SWEEP & 0xFF
    LOADI R0, (rate << 4) | ((dir & 0x01) << 3) | (shift & 0x07)
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_CHx_LEN len_frames - Set length in frames (60 Hz)
; -----------------------------------------------------------------------------
.macro APU_CH0_LEN len_frames
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH0_LEN & 0xFF
    LOADI R0, len_frames
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH1_LEN len_frames
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH1_LEN & 0xFF
    LOADI R0, len_frames
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH2_LEN len_frames
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH2_LEN & 0xFF
    LOADI R0, len_frames
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH3_LEN len_frames
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH3_LEN & 0xFF
    LOADI R0, len_frames
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_WAVE_ENV attack, decay, sustain, release - Set envelope for wave channel
; -----------------------------------------------------------------------------
.macro APU_WAVE_ENV attack, decay, sustain, release
    LOADI R6, MMIO_HI
    LOADI R7, APU_WAVE_ENV_AD & 0xFF
    LOADI R0, (attack << 4) | (decay & 0x0F)
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (sustain << 4) | (release & 0x0F)
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_WAVE_NOTE note_value, volume - Play note on wave channel
; -----------------------------------------------------------------------------
.macro APU_WAVE_NOTE note_value, volume
    LOADI R6, MMIO_HI
    LOADI R7, APU_WAVE_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, note_value >> 8
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_WAVE_OFF - Disable wave channel
; -----------------------------------------------------------------------------
.macro APU_WAVE_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_WAVE_CTRL & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_WAVE_LEN len_frames - Set length in frames (60 Hz)
; -----------------------------------------------------------------------------
.macro APU_WAVE_LEN len_frames
    LOADI R6, MMIO_HI
    LOADI R7, APU_WAVE_LEN & 0xFF
    LOADI R0, len_frames
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_WAVE_WRITE index, value - Write 4-bit sample to wave RAM (0-31)
; -----------------------------------------------------------------------------
.macro APU_WAVE_WRITE index, value
    LOADI R6, MMIO_HI
    LOADI R7, (APU_WAVE_RAM + index) & 0xFF
    LOADI R0, value
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_NOISE period, volume, mode - Start noise on channel 3
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_NOISE period, volume, mode
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH3_PERIOD & 0xFF
    LOADI R0, period
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, mode | 0x03
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_NOISE_OFF - Disable noise channel
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_NOISE_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH3_CTRL & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_NOISE_VOL vol_reg - Set noise volume from register (for fading)
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro APU_NOISE_VOL vol_reg
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH3_VOL & 0xFF
    STORE vol_reg, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; APU_ALL_OFF - Silence all channels (inlined, no nested macros)
; Clobbers: R0, R6, R7
; -----------------------------------------------------------------------------
.macro APU_ALL_OFF
    LOADI R6, MMIO_HI
    LOADI R0, 0x00
    LOADI R7, APU_CH0_CTRL & 0xFF
    STORE R0, [R6:R7]
    LOADI R7, APU_CH1_CTRL & 0xFF
    STORE R0, [R6:R7]
    LOADI R7, APU_CH2_CTRL & 0xFF
    STORE R0, [R6:R7]
    LOADI R7, APU_CH3_CTRL & 0xFF
    STORE R0, [R6:R7]
    LOADI R7, APU_WAVE_CTRL & 0xFF
    STORE R0, [R6:R7]
.endmacro
