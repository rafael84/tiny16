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
APU_STATUS_ADDR     = MMIO_BASE + 0x41  ; Status: bits 0-3=channel active flags

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

; -----------------------------------------------------------------------------
; Duty Cycle Values (for CH0_VOL/CH1_VOL bits 4-5)
; -----------------------------------------------------------------------------
APU_DUTY_12_5       = 0x00              ; 12.5% duty cycle
APU_DUTY_25         = 0x10              ; 25% duty cycle
APU_DUTY_50         = 0x20              ; 50% duty cycle (default)
APU_DUTY_75         = 0x30              ; 75% duty cycle

; -----------------------------------------------------------------------------
; Note Frequencies (11-bit values)
; Formula: freq_value = 2048 - (44100 / (16 * freq_hz))
; -----------------------------------------------------------------------------
NOTE_C3  = 1880    ; 130.81 Hz
NOTE_D3  = 1892    ; 146.83 Hz
NOTE_E3  = 1903    ; 164.81 Hz
NOTE_F3  = 1908    ; 174.61 Hz
NOTE_G3  = 1917    ; 196.00 Hz
NOTE_A3  = 1925    ; 220.00 Hz
NOTE_B3  = 1933    ; 246.94 Hz

NOTE_C4  = 1937    ; 261.63 Hz (middle C)
NOTE_D4  = 1944    ; 293.66 Hz
NOTE_E4  = 1950    ; 329.63 Hz
NOTE_F4  = 1953    ; 349.23 Hz
NOTE_G4  = 1959    ; 392.00 Hz
NOTE_A4  = 1965    ; 440.00 Hz (standard tuning)
NOTE_B4  = 1970    ; 493.88 Hz

NOTE_C5  = 1972    ; 523.25 Hz
NOTE_D5  = 1977    ; 587.33 Hz
NOTE_E5  = 1981    ; 659.25 Hz
NOTE_F5  = 1983    ; 698.46 Hz
NOTE_G5  = 1987    ; 783.99 Hz
NOTE_A5  = 1991    ; 880.00 Hz
NOTE_B5  = 1994    ; 987.77 Hz

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
    LOADI R0, 0x01
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
    LOADI R0, 0x01
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
    LOADI R0, 0x01
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
.endmacro
