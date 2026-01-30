; vim: set ft=asm:
; =============================================================================
; TINY16 INPUT UTILITIES
; =============================================================================
; Keyboard and mouse input macros
; =============================================================================

; -----------------------------------------------------------------------------
; READ_KEYS dest - Read keyboard state
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_KEYS dest
    LOADI R6, KEYS_STATE_HI
    LOADI R7, KEYS_STATE_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_KEYS_PRESSED dest - Read edge-triggered key presses
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_KEYS_PRESSED dest
    LOADI R6, KEYS_PRESSED_HI
    LOADI R7, KEYS_PRESSED_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_MOUSE_X/Y dest - Read mouse coordinates
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_MOUSE_X dest
    LOADI R6, MOUSE_X_HI
    LOADI R7, MOUSE_X_LO
    LOAD dest, [R6:R7]
.endmacro

.macro READ_MOUSE_Y dest
    LOADI R6, MOUSE_Y_HI
    LOADI R7, MOUSE_Y_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_MOUSE dest_x, dest_y - Read both mouse coordinates
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_MOUSE dest_x, dest_y
    LOADI R6, MOUSE_X_HI
    LOADI R7, MOUSE_X_LO
    LOAD dest_x, [R6:R7]
    LOADI R6, MOUSE_Y_HI
    LOADI R7, MOUSE_Y_LO
    LOAD dest_y, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_MOUSE_BUTTONS dest - Read mouse button state
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_MOUSE_BUTTONS dest
    LOADI R6, MOUSE_BUTTONS_HI
    LOADI R7, MOUSE_BUTTONS_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; SKIP_IF_CLEAR reg, mask, label - Jump to label if bit(s) are clear
; Tests if the bits specified by mask are clear (0) in reg
; Clobbers: R7
; -----------------------------------------------------------------------------
.macro SKIP_IF_CLEAR reg, mask, label
    PUSH R7
    LOADI R7, mask
    AND R7, reg
    POP R7
    JZ label
.endmacro

; -----------------------------------------------------------------------------
; SKIP_IF_SET reg, mask, label - Jump to label if bit(s) are set
; Tests if the bits specified by mask are set (1) in reg
; Clobbers: R7
; -----------------------------------------------------------------------------
.macro SKIP_IF_SET reg, mask, label
    PUSH R7
    LOADI R7, mask
    AND R7, reg
    POP R7
    JNZ label
.endmacro
