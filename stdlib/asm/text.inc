; vim: set ft=asm:
; =============================================================================
; TINY16 TEXT RENDERING UTILITIES
; =============================================================================
; Functions for rendering text as sprites using a font tileset.
; Requires: Font tiles loaded at GFX_TILES_BASE with ASCII mapping
;           (tile index = ASCII character code)
; =============================================================================

; =============================================================================
; draw_text_line - Draw null-terminated string as sprites
; =============================================================================
; Input:
;   R0 = X position (pixels)
;   R1 = Y position (pixels)
;   R3 = sprite index (starting OAM slot, will be incremented)
;   R6:R7 = address of null-terminated string
; Output:
;   R3 = updated sprite index (next available slot)
; Clobbers: R0, R1, R2, R4, R5, R6, R7 (all restored except R3)
; =============================================================================
draw_text_line:
    PUSH R0
    PUSH R1
    PUSH R2
    PUSH R4
    PUSH R5
    PUSH R6
    PUSH R7

    MOV R4, R0            ; Current X position
    MOV R5, R1            ; Y position

_dtl_loop:
    ; Load character from string
    LOAD R2, [R6:R7]+

    ; Check for null terminator
    LOADI R0, 0
    CMP R2, R0
    JZ _dtl_done

    ; Save text pointer
    PUSH R6
    PUSH R7

    ; Calculate OAM address: GFX_OAM_BASE + (sprite_index * 4)
    MOV R0, R3            ; Get sprite index
    SHL R0                ; * 2
    SHL R0                ; * 4
    LOADI R6, GFX_OAM_BASE >> 8
    MOV R7, R0            ; Low byte = sprite_index * 4

    ; Write OAM entry: Y, X, tile (ASCII), attributes
    STORE R5, [R6:R7]+    ; Y coordinate
    STORE R4, [R6:R7]+    ; X coordinate
    STORE R2, [R6:R7]+    ; Tile index (ASCII value)
    LOADI R0, 0
    STORE R0, [R6:R7]     ; Attributes (no flip, palette 0)

    ; Restore text pointer
    POP R7
    POP R6

    ; Move to next character position
    LOADI R0, 6           ; Character width + spacing (6 pixels)
    ADD R4, R0
    INC R3                ; Next sprite index

    JMP _dtl_loop

_dtl_done:
    POP R7
    POP R6
    POP R5
    POP R4
    POP R2
    POP R1
    POP R0
    RET

; =============================================================================
; TEXT_CHAR_WIDTH - Default character spacing (can be overridden)
; =============================================================================
TEXT_CHAR_WIDTH = 6
