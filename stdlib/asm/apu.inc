; vim: set ft=asm:
; =============================================================================
; TINY16 APU LIBRARY
; =============================================================================

; Waveform types (for FREQ_HI bits 4-6)
APU_WAVE_TRIANGLE   = 0x00
APU_WAVE_TSAW       = 0x10
APU_WAVE_SAW        = 0x20
APU_WAVE_SQUARE     = 0x30
APU_WAVE_PULSE      = 0x40
APU_WAVE_ORGAN      = 0x50
APU_WAVE_NOISE      = 0x60
APU_WAVE_PHASER     = 0x70

; Legacy duty constants (ignored)
APU_DUTY_12_5       = 0x00
APU_DUTY_25         = 0x00
APU_DUTY_50         = 0x00
APU_DUTY_75         = 0x00

; -----------------------------------------------------------------------------
; Registers
; -----------------------------------------------------------------------------
APU_CTRL_ADDR       = MMIO_BASE + 0x40
APU_STATUS_ADDR     = MMIO_BASE + 0x41

; Channel 0
APU_CH0_FREQ_LO     = MMIO_BASE + 0x42
APU_CH0_FREQ_HI     = MMIO_BASE + 0x43
APU_CH0_VOL         = MMIO_BASE + 0x44
APU_CH0_CTRL        = MMIO_BASE + 0x45
APU_CH0_ENV_AD      = MMIO_BASE + 0x46
APU_CH0_ENV_SR      = MMIO_BASE + 0x47

; Channel 1
APU_CH1_FREQ_LO     = MMIO_BASE + 0x48
APU_CH1_FREQ_HI     = MMIO_BASE + 0x49
APU_CH1_VOL         = MMIO_BASE + 0x4A
APU_CH1_CTRL        = MMIO_BASE + 0x4B
APU_CH1_ENV_AD      = MMIO_BASE + 0x4C
APU_CH1_ENV_SR      = MMIO_BASE + 0x4D

; Channel 2
APU_CH2_FREQ_LO     = MMIO_BASE + 0x4E
APU_CH2_FREQ_HI     = MMIO_BASE + 0x4F
APU_CH2_VOL         = MMIO_BASE + 0x50
APU_CH2_CTRL        = MMIO_BASE + 0x51
APU_CH2_ENV_AD      = MMIO_BASE + 0x52
APU_CH2_ENV_SR      = MMIO_BASE + 0x53

; Channel 3
APU_CH3_FREQ_LO     = MMIO_BASE + 0x54
APU_CH3_FREQ_HI     = MMIO_BASE + 0x55
APU_CH3_VOL         = MMIO_BASE + 0x56
APU_CH3_CTRL        = MMIO_BASE + 0x57
APU_CH3_ENV_AD      = MMIO_BASE + 0x58
APU_CH3_ENV_SR      = MMIO_BASE + 0x59

; Length registers
APU_CH0_LEN         = MMIO_BASE + 0x5A
APU_CH1_LEN         = MMIO_BASE + 0x5B
APU_CH2_LEN         = MMIO_BASE + 0x5C
APU_CH3_LEN         = MMIO_BASE + 0x5D

; -----------------------------------------------------------------------------
; Note frequencies (11-bit): freq_value = 2048 - (44100 / freq_hz)
; -----------------------------------------------------------------------------
NOTE_C3  = 1711
NOTE_CS3 = 1730
NOTE_D3  = 1748
NOTE_DS3 = 1765
NOTE_E3  = 1780
NOTE_F3  = 1795
NOTE_FS3 = 1810
NOTE_G3  = 1823
NOTE_GS3 = 1836
NOTE_A3  = 1848
NOTE_AS3 = 1859
NOTE_B3  = 1869

NOTE_C4  = 1879
NOTE_CS4 = 1889
NOTE_D4  = 1898
NOTE_DS4 = 1906
NOTE_E4  = 1914
NOTE_F4  = 1922
NOTE_FS4 = 1929
NOTE_G4  = 1935
NOTE_GS4 = 1942
NOTE_A4  = 1948
NOTE_AS4 = 1953
NOTE_B4  = 1959

NOTE_C5  = 1964
NOTE_CS5 = 1968
NOTE_D5  = 1973
NOTE_DS5 = 1977
NOTE_E5  = 1981
NOTE_F5  = 1985
NOTE_FS5 = 1988
NOTE_G5  = 1992
NOTE_GS5 = 1995
NOTE_A5  = 1998
NOTE_AS5 = 2001
NOTE_B5  = 2003

; -----------------------------------------------------------------------------
; Macros
; -----------------------------------------------------------------------------

; APU_INIT master_vol - Clobbers: R0, R6, R7
.macro APU_INIT master_vol
    LOADI R6, MMIO_HI
    LOADI R7, APU_CTRL_ADDR & 0xFF
    LOADI R0, (master_vol << 4) | 0x01
    STORE R0, [R6:R7]
.endmacro

; APU_OFF - Clobbers: R0, R6, R7
.macro APU_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_CTRL_ADDR & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

; APU_CHx_PLAY note_value, volume, waveform - Clobbers: R0, R6, R7
.macro APU_CH0_PLAY note_value, volume, waveform
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH0_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (note_value >> 8) | waveform
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH1_PLAY note_value, volume, waveform
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH1_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (note_value >> 8) | waveform
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH2_PLAY note_value, volume, waveform
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH2_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (note_value >> 8) | waveform
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH3_PLAY note_value, volume, waveform
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH3_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (note_value >> 8) | waveform
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

; APU_CHx_OFF - Clobbers: R0, R6, R7
.macro APU_CH0_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH0_CTRL & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH1_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH1_CTRL & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH2_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH2_CTRL & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH3_OFF
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH3_CTRL & 0xFF
    LOADI R0, 0x00
    STORE R0, [R6:R7]
.endmacro

; APU_CHx_ENV attack, decay, sustain, release
.macro APU_CH0_ENV attack, decay, sustain, release
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH0_ENV_AD & 0xFF
    LOADI R0, (attack << 4) | (decay & 0x0F)
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (sustain << 4) | (release & 0x0F)
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH1_ENV attack, decay, sustain, release
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH1_ENV_AD & 0xFF
    LOADI R0, (attack << 4) | (decay & 0x0F)
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (sustain << 4) | (release & 0x0F)
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH2_ENV attack, decay, sustain, release
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH2_ENV_AD & 0xFF
    LOADI R0, (attack << 4) | (decay & 0x0F)
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (sustain << 4) | (release & 0x0F)
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH3_ENV attack, decay, sustain, release
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH3_ENV_AD & 0xFF
    LOADI R0, (attack << 4) | (decay & 0x0F)
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (sustain << 4) | (release & 0x0F)
    STORE R0, [R6:R7]
.endmacro

; APU_CHx_LEN len_frames
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

; APU_NOISE period, volume - Clobbers: R0, R6, R7
.macro APU_NOISE period, volume
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH3_FREQ_LO & 0xFF
    LOADI R0, period
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, APU_WAVE_NOISE
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

; APU_ALL_OFF - Clobbers: R0, R6, R7
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
.endmacro

; Legacy macros (duty param ignored)
.macro APU_CH0_NOTE note_value, volume, duty
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH0_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (note_value >> 8) | APU_WAVE_PULSE
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH1_NOTE note_value, volume, duty
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH1_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (note_value >> 8) | APU_WAVE_PULSE
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro

.macro APU_CH2_NOTE note_value, volume
    LOADI R6, MMIO_HI
    LOADI R7, APU_CH2_FREQ_LO & 0xFF
    LOADI R0, note_value & 0xFF
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, (note_value >> 8) | APU_WAVE_TRIANGLE
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, volume
    STORE R0, [R6:R7]
    INC R7
    LOADI R0, 0x03
    STORE R0, [R6:R7]
.endmacro
