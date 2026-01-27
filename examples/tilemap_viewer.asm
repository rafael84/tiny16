.include "../stdlib/tiny16.inc"

section .code

START:
    ; Draw 16×8 grid centered on screen (16×16 visible tiles)
    ; Starting tilemap position: X=0, Y=4 (centered vertically)
    ; Starting address = 0x7000 + (4 * 32 + 0) * 2 = 0x7000 + 0x100 = 0x7100
    LOADI R6, 0x71        ; High byte of start address
    LOADI R7, 0x00        ; Low byte of start address
    LOADI R3, 0           ; Tile counter
    LOADI R4, 8           ; Row counter (8 rows)

ROW_LOOP:
    LOADI R2, 16          ; Column counter (16 tiles per row)
COL_LOOP:
    MOV   R0, R3          ; Current tile index
    STORE R0, [R6:R7]+    ; Store tile index, increment address
    LOADI R0, 0           ; Tile attribute (0)
    STORE R0, [R6:R7]+    ; Store attribute, increment address
    INC   R3              ; Next tile
    DEC   R2              ; Decrement column counter
    JNZ   COL_LOOP        ; Continue until 16 tiles done

    ; Skip to next row: add (32-16)*2 = 32 bytes to address
    LOADI R0, 32
    ADD   R7, R0          ; Add 32 to low byte
    LOADI R0, 0
    ADC   R6, R0          ; Add carry to high byte

    DEC   R4              ; Decrement row counter
    JNZ   ROW_LOOP        ; Continue until 8 rows done

    ; Enable rendering
    LOADI R6, 0xBF
    LOADI R7, 0x30
    LOADI R0, 0x01
    STORE R0, [R6:R7]

LOOP:
    ; Trigger PPU render
    LOADI R6, 0xBF
    LOADI R7, 0x30
    LOADI R0, 0x81
    STORE R0, [R6:R7]
    
    ; Wait vsync
    PUSH  R0
    PUSH  R1
    PUSH  R6
    PUSH  R7
    LOADI R6, 0xBF
    LOADI R7, 0x22
    LOAD  R0, [R6:R7]
WAIT_LOOP:
    LOAD  R1, [R6:R7]
    CMP   R0, R1
    JZ    WAIT_LOOP
    POP   R7
    POP   R6
    POP   R1
    POP   R0
    
    JMP LOOP

section .data

.include "tileset.inc"
