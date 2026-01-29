; vim: set ft=asm:
; =============================================================================
; TINY16 STANDARD LIBRARY
; =============================================================================
;
; A comprehensive collection of macros and constants for Tiny16 assembly.
; This file contains everything you need - no external documentation required.
;
; QUICK START:
;   .include "../stdlib/tiny16.inc"
;
;   section .code
;   START:
;       CLEAR_SCREEN 0x03        ; Dark blue background
;       STORE16I 42, 0x4000      ; Store value at 0x4000
;       LOAD16 R0, 0x4000        ; Load it back
;       HALT
;
; CONVENTIONS:
;   - R6:R7 = Standard address pointer pair
;   - R0 = Return value (where applicable)
;   - R5, R7 = Common temporary registers
;   - Most macros preserve registers (documented if not)
;   - Register pairs: high byte first, low byte second (e.g., R0:R1 = 0x1234)
;
; EXPRESSION SUPPORT:
;   The assembler supports expressions in immediate values. Macros automatically
;   split 16-bit addresses using >> and & operators:
;     LOAD16 R0, 0x4010        ; R0 = [0x4010]
;     SETADDR 0x4020           ; R6:R7 = 0x4020
;     MEMCPY 0x5000, 0x4000, 50  ; Copy memory
;
; TABLE OF CONTENTS:
;   1. Hardware Constants (MMIO addresses, graphics memory)
;   2. Memory & Register Utilities (SETADDR, LOAD16, STORE16, STORE16I, LOAD16I)
;   3. Stack Operations (multi-register push/pop)
;   4. Arithmetic & Logic (immediate ops, fast multiply/divide, clamping)
;   5. 16-bit Operations (add/sub/cmp with carry/borrow)
;   6. Input Utilities (keyboard, mouse)
;   7. VSync & Timing (frame counter, WAIT_VSYNC)
;   8. PPU & Graphics (sprite rendering control)
;   9. OAM/Sprite Utilities (sprite management)
;   10. Memory Operations (MEMSET, MEMCPY, screen clear)
;   11. Framebuffer Utilities (pixel drawing, address calculation)
;   12. Debugging & Development (nop, breakpoint)
;
; =============================================================================

; =============================================================================
; 1. HARDWARE CONSTANTS - MMIO Memory Map
; =============================================================================

MMIO_BASE           = 0xBF00
MMIO_HI             = MMIO_BASE >> 8

; Input devices (read-only)
KEYS_STATE_ADDR     = MMIO_BASE + 0x00  ; Current keyboard state
KEYS_PRESSED_ADDR   = MMIO_BASE + 0x01  ; Edge-triggered key presses
MOUSE_X_ADDR        = MMIO_BASE + 0x02  ; Mouse X coordinate (0-127)
MOUSE_Y_ADDR        = MMIO_BASE + 0x03  ; Mouse Y coordinate (0-127)
MOUSE_BUTTONS_ADDR  = MMIO_BASE + 0x04  ; Mouse button state

; Display timing (read-only)
FRAME_COUNT_ADDR    = MMIO_BASE + 0x22  ; Increments at 60 Hz

; PPU control (write-only)
PPU_CTRL_ADDR       = MMIO_BASE + 0x30  ; Graphics control register

; Pre-computed hi/lo bytes for fast MMIO access
KEYS_STATE_HI       = KEYS_STATE_ADDR >> 8
KEYS_STATE_LO       = KEYS_STATE_ADDR & 0xFF
KEYS_PRESSED_HI     = KEYS_PRESSED_ADDR >> 8
KEYS_PRESSED_LO     = KEYS_PRESSED_ADDR & 0xFF
MOUSE_X_HI          = MOUSE_X_ADDR >> 8
MOUSE_X_LO          = MOUSE_X_ADDR & 0xFF
MOUSE_Y_HI          = MOUSE_Y_ADDR >> 8
MOUSE_Y_LO          = MOUSE_Y_ADDR & 0xFF
MOUSE_BUTTONS_HI    = MOUSE_BUTTONS_ADDR >> 8
MOUSE_BUTTONS_LO    = MOUSE_BUTTONS_ADDR & 0xFF
FRAME_COUNT_HI      = FRAME_COUNT_ADDR >> 8
FRAME_COUNT_LO      = FRAME_COUNT_ADDR & 0xFF
PPU_CTRL_HI         = PPU_CTRL_ADDR >> 8
PPU_CTRL_LO         = PPU_CTRL_ADDR & 0xFF

; Graphics memory map
FRAMEBUFFER_BASE    = 0xC000            ; 128x128 framebuffer (RGB332)
FRAMEBUFFER_HI      = FRAMEBUFFER_BASE >> 8
FRAMEBUFFER_LO      = FRAMEBUFFER_BASE & 0xFF

GFX_TILES_BASE      = 0x5000            ; Tile data (8x8, 4bpp)
GFX_OAM_BASE        = 0x7800            ; Object Attribute Memory (sprites)
GFX_PALETTE_BASE    = 0x7900            ; Color palette

GFX_OAM_SPRITE_COUNT = 64               ; Maximum sprites
GFX_OAM_ENTRY_SIZE   = 4                ; Bytes per OAM entry

; PPU control bits
PPU_CTRL_SPRITES    = 0x02              ; Enable sprite rendering
PPU_CTRL_RENDER_NOW = 0x80              ; Trigger immediate render
PPU_CTRL_SPRITES_ON = PPU_CTRL_SPRITES | PPU_CTRL_RENDER_NOW   ; 0x82

; =============================================================================
; 2. MEMORY & REGISTER UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; SETADDR addr16 - Set R6:R7 to 16-bit address
; Uses expressions to automatically split address into high/low bytes
; Clobbers: R6, R7
; Example:
;   DATA_ADDR = 0x4010
;   SETADDR DATA_ADDR       ; R6:R7 = 0x4010
;   SETADDR 0x4020          ; R6:R7 = 0x4020
;   LOAD R0, [R6:R7]        ; Load from address
; -----------------------------------------------------------------------------
.macro SETADDR addr16
    LOADI R6, addr16 >> 8
    LOADI R7, addr16 & 0xFF
.endmacro

; -----------------------------------------------------------------------------
; LOAD16 dest, addr16 - Load from 16-bit address
; Uses expressions to automatically split address into high/low bytes
; Clobbers: R6, R7
; Example:
;   DATA_ADDR = 0x4000
;   LOAD16 R0, DATA_ADDR      ; R0 = [0x4000]
;   LOAD16 R1, 0x4010         ; R1 = [0x4010]
; -----------------------------------------------------------------------------
.macro LOAD16 dest, addr16
    LOADI R6, addr16 >> 8
    LOADI R7, addr16 & 0xFF
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; STORE16 src, addr16 - Store register to 16-bit address
; Uses expressions to automatically split address into high/low bytes
; Clobbers: R6, R7
; Example:
;   DATA_ADDR = 0x4000
;   LOADI R0, 42
;   STORE16 R0, DATA_ADDR     ; [0x4000] = 42
;   STORE16 R1, 0x4010        ; [0x4010] = R1
; -----------------------------------------------------------------------------
.macro STORE16 src, addr16
    LOADI R6, addr16 >> 8
    LOADI R7, addr16 & 0xFF
    STORE src, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; STORE16I imm, addr16 - Store immediate to 16-bit address
; Uses expressions to automatically split address into high/low bytes
; Clobbers: R0, R6, R7
; Example:
;   DATA_ADDR = 0x4000
;   STORE16I 0xAA, DATA_ADDR      ; [0x4000] = 0xAA
;   STORE16I 42, 0x4010           ; [0x4010] = 42
; -----------------------------------------------------------------------------
.macro STORE16I imm, addr16
    LOADI R0, imm
    LOADI R6, addr16 >> 8
    LOADI R7, addr16 & 0xFF
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; CLEAR reg - Zero a register (faster than LOADI reg, 0)
; Clobbers: reg
; Example:
;   CLEAR R0    ; R0 = 0
; -----------------------------------------------------------------------------
.macro CLEAR reg
    XOR reg, reg
.endmacro

; -----------------------------------------------------------------------------
; SET reg, value - Set register to immediate (semantic alias for LOADI)
; Clobbers: reg
; Example:
;   SET R0, 42    ; R0 = 42 (clearer intent than LOADI)
; -----------------------------------------------------------------------------
.macro SET reg, value
    LOADI reg, value
.endmacro

; -----------------------------------------------------------------------------
; NEGATE reg - Two's complement negation (reg = -reg)
; Clobbers: reg
; Example:
;   LOADI R0, 5
;   NEGATE R0    ; R0 = -5 (0xFB in 8-bit)
; -----------------------------------------------------------------------------
.macro NEGATE reg
    XOR reg, reg
    INC reg
.endmacro

; -----------------------------------------------------------------------------
; SKIP_IF_CLEAR reg, bitmask, label - Test bit and jump if clear
; Preserves: all registers
; Example:
;   READ_KEYS R0
;   SKIP_IF_CLEAR R0, 0x01, NO_UP_KEY    ; Jump if UP not pressed
; NO_UP_KEY:
; -----------------------------------------------------------------------------
.macro SKIP_IF_CLEAR reg, bitmask, label
    PUSH R3
    PUSH R4
    MOV R3, reg
    LOADI R4, bitmask
    AND R3, R4
    POP R4
    POP R3
    JZ label
.endmacro

; -----------------------------------------------------------------------------
; SKIP_IF_SET reg, bitmask, label - Test bit and jump if set
; Preserves: all registers
; Example:
;   READ_KEYS R0
;   SKIP_IF_SET R0, 0x01, UP_PRESSED     ; Jump if UP is pressed
; UP_PRESSED:
; -----------------------------------------------------------------------------
.macro SKIP_IF_SET reg, bitmask, label
    PUSH R3
    PUSH R4
    MOV R3, reg
    LOADI R4, bitmask
    AND R3, R4
    POP R4
    POP R3
    JNZ label
.endmacro

; =============================================================================
; 3. STACK OPERATIONS - Multi-register Push/Pop
; =============================================================================

; -----------------------------------------------------------------------------
; PUSH2/POP2 - Push/pop 2 registers
; Note: POP order is reversed to match stack semantics
; Example:
;   PUSH2 R0, R1    ; Push R0, then R1
;   ; ... use R0, R1 ...
;   POP2 R0, R1     ; Pop R1, then R0 (automatic reversal)
; -----------------------------------------------------------------------------
.macro PUSH2 r1, r2
    PUSH r1
    PUSH r2
.endmacro

.macro POP2 r2, r1
    POP r1
    POP r2
.endmacro

; -----------------------------------------------------------------------------
; PUSH3/POP3 - Push/pop 3 registers
; Example:
;   PUSH3 R0, R1, R2
;   POP3 R0, R1, R2
; -----------------------------------------------------------------------------
.macro PUSH3 r1, r2, r3
    PUSH r1
    PUSH r2
    PUSH r3
.endmacro

.macro POP3 r3, r2, r1
    POP r1
    POP r2
    POP r3
.endmacro

; -----------------------------------------------------------------------------
; PUSH4/POP4 - Push/pop 4 registers
; Example:
;   PUSH4 R0, R1, R2, R3
;   POP4 R0, R1, R2, R3
; -----------------------------------------------------------------------------
.macro PUSH4 r1, r2, r3, r4
    PUSH r1
    PUSH r2
    PUSH r3
    PUSH r4
.endmacro

.macro POP4 r4, r3, r2, r1
    POP r1
    POP r2
    POP r3
    POP r4
.endmacro

; =============================================================================
; 4. ARITHMETIC & LOGIC UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; ADDI reg, imm - Add immediate value to register
; Clobbers: R7 (temporary)
; Example:
;   LOADI R0, 10
;   ADDI R0, 5    ; R0 = 15
; -----------------------------------------------------------------------------
.macro ADDI reg, imm
    PUSH R7
    LOADI R7, imm
    ADD reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; SUBI reg, imm - Subtract immediate value from register
; Clobbers: R7 (temporary)
; Example:
;   LOADI R0, 20
;   SUBI R0, 5    ; R0 = 15
; -----------------------------------------------------------------------------
.macro SUBI reg, imm
    PUSH R7
    LOADI R7, imm
    SUB reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; CMPI reg, imm - Compare register with immediate value
; Clobbers: R7 (temporary), flags
; Example:
;   LOADI R0, 42
;   CMPI R0, 42
;   JZ EQUAL    ; Jump if R0 == 42
; -----------------------------------------------------------------------------
.macro CMPI reg, imm
    PUSH R7
    LOADI R7, imm
    CMP reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; ANDI reg, imm - Bitwise AND register with immediate value
; Clobbers: R7 (temporary)
; Example:
;   LOADI R0, 0xFF
;   ANDI R0, 0x0F    ; R0 = 0x0F (mask lower 4 bits)
; -----------------------------------------------------------------------------
.macro ANDI reg, imm
    PUSH R7
    LOADI R7, imm
    AND reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; ORI reg, imm - Bitwise OR register with immediate value
; Clobbers: R7 (temporary)
; Example:
;   LOADI R0, 0x10
;   ORI R0, 0x01     ; R0 = 0x11 (set bit 0)
; -----------------------------------------------------------------------------
.macro ORI reg, imm
    PUSH R7
    LOADI R7, imm
    OR reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; XORI reg, imm - Bitwise XOR register with immediate value
; Clobbers: R7 (temporary)
; Example:
;   LOADI R0, 0xFF
;   XORI R0, 0xFF    ; R0 = 0x00 (toggle all bits)
; -----------------------------------------------------------------------------
.macro XORI reg, imm
    PUSH R7
    LOADI R7, imm
    XOR reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; INC_BY reg, count - Increment register by N
; Clobbers: R7 (temporary)
; Example:
;   LOADI R0, 10
;   INC_BY R0, 5     ; R0 = 15
; -----------------------------------------------------------------------------
.macro INC_BY reg, count
    PUSH R7
    LOADI R7, count
    ADD reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; DEC_BY reg, count - Decrement register by N
; Clobbers: R7 (temporary)
; Example:
;   LOADI R0, 20
;   DEC_BY R0, 5     ; R0 = 15
; -----------------------------------------------------------------------------
.macro DEC_BY reg, count
    PUSH R7
    LOADI R7, count
    SUB reg, R7
    POP R7
.endmacro

; -----------------------------------------------------------------------------
; Fast Multiplication (powers of 2 only)
; MUL2 reg - Multiply by 2 (left shift once)
; MUL4 reg - Multiply by 4 (left shift twice)
; MUL8 reg - Multiply by 8 (left shift three times)
; Example:
;   LOADI R0, 5
;   MUL4 R0    ; R0 = 20
; -----------------------------------------------------------------------------
.macro MUL2 reg
    ADD reg, reg
.endmacro

.macro MUL4 reg
    ADD reg, reg
    ADD reg, reg
.endmacro

.macro MUL8 reg
    ADD reg, reg
    ADD reg, reg
    ADD reg, reg
.endmacro

; -----------------------------------------------------------------------------
; Fast Division (powers of 2 only)
; DIV2 reg - Divide by 2 (right shift once)
; DIV4 reg - Divide by 4 (right shift twice)
; DIV8 reg - Divide by 8 (right shift three times)
; Example:
;   LOADI R0, 32
;   DIV4 R0    ; R0 = 8
; -----------------------------------------------------------------------------
.macro DIV2 reg
    SHR reg
.endmacro

.macro DIV4 reg
    SHR reg
    SHR reg
.endmacro

.macro DIV8 reg
    SHR reg
    SHR reg
    SHR reg
.endmacro

; -----------------------------------------------------------------------------
; CLAMP_MAX reg, max_val - Clamp register to maximum [0, max]
; Clobbers: R5 (temporary)
; Example:
;   LOADI R0, 200
;   CLAMP_MAX R0, 100    ; R0 = 100 (clamped)
; -----------------------------------------------------------------------------
.macro CLAMP_MAX reg, max_val
    PUSH R5
    LOADI R5, max_val
    CMP reg, R5
    JC @skip
    MOV reg, R5
@skip:
    POP R5
.endmacro

; -----------------------------------------------------------------------------
; CLAMP_MIN reg, min_val - Clamp register to minimum [min, 255]
; Clobbers: R5 (temporary)
; Example:
;   LOADI R0, 5
;   CLAMP_MIN R0, 10     ; R0 = 10 (clamped)
; -----------------------------------------------------------------------------
.macro CLAMP_MIN reg, min_val
    PUSH R5
    LOADI R5, min_val
    CMP reg, R5
    JNC @skip
    MOV reg, R5
@skip:
    POP R5
.endmacro

; -----------------------------------------------------------------------------
; CLAMP_RANGE reg, min_val, max_val - Clamp to arbitrary range [min, max]
; Clobbers: R5 (temporary)
; Example:
;   LOADI R0, 200
;   CLAMP_RANGE R0, 10, 100    ; R0 = 100
; -----------------------------------------------------------------------------
.macro CLAMP_RANGE reg, min_val, max_val
    PUSH R5
    ; Clamp to minimum
    LOADI R5, min_val
    CMP reg, R5
    JNC @skip_min
    MOV reg, R5
@skip_min:
    ; Clamp to maximum
    LOADI R5, max_val
    CMP reg, R5
    JC @skip_max
    MOV reg, R5
@skip_max:
    POP R5
.endmacro

; =============================================================================
; 5. 16-BIT OPERATIONS
; =============================================================================
; Convention: 16-bit values stored in register pairs (high:low)
; Example: R0:R1 = 0x1234 means R0=0x12, R1=0x34

; -----------------------------------------------------------------------------
; ADD16 dst_hi, dst_lo, src_hi, src_lo - 16-bit addition with carry
; dst = dst + src
; Clobbers: flags
; Example:
;   LOADI R0, 0x12    ; R0:R1 = 0x1234
;   LOADI R1, 0x34
;   LOADI R2, 0x00    ; R2:R3 = 0x0042
;   LOADI R3, 0x42
;   ADD16 R0, R1, R2, R3    ; R0:R1 = 0x1276
; -----------------------------------------------------------------------------
.macro ADD16 dst_hi, dst_lo, src_hi, src_lo
    ADD dst_lo, src_lo
    JNC @no_carry
    INC dst_hi
@no_carry:
    ADD dst_hi, src_hi
.endmacro

; -----------------------------------------------------------------------------
; SUB16 dst_hi, dst_lo, src_hi, src_lo - 16-bit subtraction with borrow
; dst = dst - src
; Clobbers: flags
; Example:
;   LOADI R0, 0x50    ; R0:R1 = 0x5000
;   LOADI R1, 0x00
;   LOADI R2, 0x02    ; R2:R3 = 0x0234
;   LOADI R3, 0x34
;   SUB16 R0, R1, R2, R3    ; R0:R1 = 0x4DCC
; -----------------------------------------------------------------------------
.macro SUB16 dst_hi, dst_lo, src_hi, src_lo
    SUB dst_lo, src_lo
    JNC @no_borrow
    DEC dst_hi
@no_borrow:
    SUB dst_hi, src_hi
.endmacro

; -----------------------------------------------------------------------------
; INC16 reg_hi, reg_lo - 16-bit increment
; Clobbers: flags
; Example:
;   LOADI R0, 0x00
;   LOADI R1, 0xFF
;   INC16 R0, R1    ; R0:R1 = 0x0100
; -----------------------------------------------------------------------------
.macro INC16 reg_hi, reg_lo
    INC reg_lo
    JNZ @done
    INC reg_hi
@done:
.endmacro

; -----------------------------------------------------------------------------
; DEC16 reg_hi, reg_lo - 16-bit decrement
; Clobbers: flags, R0 (temporary)
; Example:
;   LOADI R0, 0x01
;   LOADI R1, 0x00
;   DEC16 R0, R1    ; R0:R1 = 0x00FF
; -----------------------------------------------------------------------------
.macro DEC16 reg_hi, reg_lo
    DEC reg_lo
    LOADI R0, 0xFF
    CMP reg_lo, R0
    JNZ @done
    DEC reg_hi
@done:
.endmacro

; -----------------------------------------------------------------------------
; CMP16 cmp_hi, cmp_lo, val_hi, val_lo - 16-bit comparison
; Sets flags as if: (cmp_hi:cmp_lo) - (val_hi:val_lo)
; Clobbers: flags
; Example:
;   LOADI R0, 0x12
;   LOADI R1, 0x34
;   LOADI R2, 0x12
;   LOADI R3, 0x34
;   CMP16 R0, R1, R2, R3
;   JZ EQUAL    ; Jump if R0:R1 == R2:R3
; -----------------------------------------------------------------------------
.macro CMP16 cmp_hi, cmp_lo, val_hi, val_lo
    CMP cmp_hi, val_hi
    JNZ @done
    CMP cmp_lo, val_lo
@done:
.endmacro

; -----------------------------------------------------------------------------
; LOAD16I reg_hi, reg_lo, value16 - Load 16-bit immediate into register pair
; Uses expressions to automatically split 16-bit value into high/low bytes
; Example:
;   ADDR = 0x1234
;   LOAD16I R0, R1, ADDR         ; R0:R1 = 0x1234
;   LOAD16I R6, R7, 0x4020       ; R6:R7 = 0x4020
; -----------------------------------------------------------------------------
.macro LOAD16I reg_hi, reg_lo, value16
    LOADI reg_hi, value16 >> 8
    LOADI reg_lo, value16 & 0xFF
.endmacro

; =============================================================================
; 6. INPUT UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; READ_KEYS dest - Read keyboard state
; Clobbers: R6, R7
; Example:
;   READ_KEYS R0
;   LOADI R1, 0x01    ; UP key bit
;   AND R0, R1
;   JNZ UP_PRESSED
; -----------------------------------------------------------------------------
.macro READ_KEYS dest
    LOADI R6, KEYS_STATE_HI
    LOADI R7, KEYS_STATE_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_KEYS_PRESSED dest - Read edge-triggered key presses
; Returns 1 for keys pressed this frame (not held from previous frame)
; Clobbers: R6, R7
; Example:
;   READ_KEYS_PRESSED R0
;   LOADI R1, 0x80    ; START key
;   AND R0, R1
;   JNZ START_PRESSED    ; Fires once per press
; -----------------------------------------------------------------------------
.macro READ_KEYS_PRESSED dest
    LOADI R6, KEYS_PRESSED_HI
    LOADI R7, KEYS_PRESSED_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_MOUSE_X dest - Read mouse X coordinate (0-127)
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_MOUSE_X dest
    LOADI R6, MOUSE_X_HI
    LOADI R7, MOUSE_X_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_MOUSE_Y dest - Read mouse Y coordinate (0-127)
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_MOUSE_Y dest
    LOADI R6, MOUSE_Y_HI
    LOADI R7, MOUSE_Y_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_MOUSE_BUTTONS dest - Read mouse button state
; Bit 0 = left button, Bit 1 = right button
; Clobbers: R6, R7
; -----------------------------------------------------------------------------
.macro READ_MOUSE_BUTTONS dest
    LOADI R6, MOUSE_BUTTONS_HI
    LOADI R7, MOUSE_BUTTONS_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; READ_MOUSE dest_x, dest_y - Read both mouse coordinates at once
; Clobbers: R6, R7
; Example:
;   READ_MOUSE R0, R1    ; R0=x, R1=y
; -----------------------------------------------------------------------------
.macro READ_MOUSE dest_x, dest_y
    LOADI R6, MOUSE_X_HI
    LOADI R7, MOUSE_X_LO
    LOAD dest_x, [R6:R7]
    LOADI R6, MOUSE_Y_HI
    LOADI R7, MOUSE_Y_LO
    LOAD dest_y, [R6:R7]
.endmacro

; =============================================================================
; 7. VSYNC & TIMING UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; READ_FRAME_COUNT dest - Read frame counter (increments at 60 Hz)
; Clobbers: R6, R7
; Example:
;   READ_FRAME_COUNT R0
;   LOADI R1, 0x07
;   AND R0, R1    ; R0 = frame % 8 (for 8-frame animation)
; -----------------------------------------------------------------------------
.macro READ_FRAME_COUNT dest
    LOADI R6, FRAME_COUNT_HI
    LOADI R7, FRAME_COUNT_LO
    LOAD dest, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; WAIT_VSYNC last_frame_addr - Wait for next frame
; Spins until FRAME_COUNT changes, then updates last_frame
; Uses expressions to automatically split address
; Clobbers: R0, R1, R6, R7
; Example:
;   LAST_FRAME_ADDR = 0x4000
;   section .data
;   ORG LAST_FRAME_ADDR
;   last_frame: DB 0
;
;   section .code
;   WAIT_VSYNC LAST_FRAME_ADDR    ; Wait for 60 Hz tick
; -----------------------------------------------------------------------------
.macro WAIT_VSYNC last_frame_addr
@loop:
    LOADI R6, FRAME_COUNT_HI
    LOADI R7, FRAME_COUNT_LO
    LOAD R0, [R6:R7]
    LOADI R6, last_frame_addr >> 8
    LOADI R7, last_frame_addr & 0xFF
    LOAD R1, [R6:R7]
    CMP R0, R1
    JZ @loop
    STORE R0, [R6:R7]
.endmacro

; =============================================================================
; 8. PPU & GRAPHICS UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; PPU_RENDER flags - Set PPU control register with custom flags
; Clobbers: R0, R6, R7
; Example:
;   PPU_RENDER 0x82    ; Enable sprites + render now
; -----------------------------------------------------------------------------
.macro PPU_RENDER flags
    LOADI R0, flags
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; PPU_SPRITES_ON - Enable sprite rendering (most common operation)
; Clobbers: R0, R6, R7
; Example:
;   PPU_SPRITES_ON    ; Show sprites
; -----------------------------------------------------------------------------
.macro PPU_SPRITES_ON
    LOADI R0, PPU_CTRL_SPRITES_ON
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; PPU_OFF - Disable all rendering
; Clobbers: R0, R6, R7
; Example:
;   PPU_OFF    ; Turn off display
; -----------------------------------------------------------------------------
.macro PPU_OFF
    XOR R0, R0
    LOADI R6, PPU_CTRL_HI
    LOADI R7, PPU_CTRL_LO
    STORE R0, [R6:R7]
.endmacro

; =============================================================================
; 9. OAM/SPRITE UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; OAM_WRITE_SPRITE y_reg, x_reg, tile, attr - Write sprite to OAM
; Requires: R6:R7 already set to OAM entry address
; Format: [Y, X, tile_index, attributes]
; Clobbers: R0, advances R6:R7 by 4 bytes
; Example:
;   LOADI R6, 0x78    ; OAM base = 0x7800
;   LOADI R7, 0x00
;   LOADI R0, 64      ; x = 64
;   LOADI R1, 64      ; y = 64
;   OAM_WRITE_SPRITE R1, R0, 0, 0    ; Sprite 0 at (64,64), tile 0
; -----------------------------------------------------------------------------
.macro OAM_WRITE_SPRITE y_reg, x_reg, tile, attr
    STORE y_reg, [R6:R7]+
    STORE x_reg, [R6:R7]+
    LOADI R0, tile
    STORE R0, [R6:R7]+
    LOADI R0, attr
    STORE R0, [R6:R7]
.endmacro

; -----------------------------------------------------------------------------
; OAM_HIDE_SPRITE - Hide sprite by setting Y to 0xFF
; Requires: R6:R7 set to OAM entry address
; Clobbers: R0, advances R7 by 4
; Example:
;   LOADI R6, 0x78
;   LOADI R7, 0x00    ; Sprite 0
;   OAM_HIDE_SPRITE   ; Hide it
; -----------------------------------------------------------------------------
.macro OAM_HIDE_SPRITE
    LOADI R0, 0xFF
    STORE R0, [R6:R7]
    INC R7
    INC R7
    INC R7
    INC R7
.endmacro

; -----------------------------------------------------------------------------
; OAM_SET_SPRITE sprite_idx - Set R6:R7 to OAM entry for sprite index
; Result: R6:R7 = 0x7800 + (sprite_idx * 4)
; Clobbers: R6, R7
; Example:
;   OAM_SET_SPRITE 0    ; R6:R7 = 0x7800 (sprite 0)
;   OAM_SET_SPRITE 1    ; R6:R7 = 0x7804 (sprite 1)
; -----------------------------------------------------------------------------
.macro OAM_SET_SPRITE sprite_idx
    LOADI R6, GFX_OAM_BASE >> 8
    LOADI R7, (GFX_OAM_BASE & 0xFF) + (sprite_idx * 4)
.endmacro

; =============================================================================
; 10. MEMORY OPERATIONS
; =============================================================================

; -----------------------------------------------------------------------------
; MEMSET addr16, count, value - Fill memory with value
; Uses expressions to automatically split address into high/low bytes
; Clobbers: R0, R1, R6, R7
; Example:
;   BUFFER_ADDR = 0x4100
;   MEMSET BUFFER_ADDR, 100, 0xAA    ; Fill 100 bytes with 0xAA
;   MEMSET 0x5000, 50, 0xFF          ; Fill 50 bytes with 0xFF
; -----------------------------------------------------------------------------
.macro MEMSET addr16, count, value
    LOADI R6, addr16 >> 8
    LOADI R7, addr16 & 0xFF
    LOADI R0, value
    LOADI R1, count
@loop:
    STORE R0, [R6:R7]+
    DEC R1
    JNZ @loop
.endmacro

; -----------------------------------------------------------------------------
; MEMCPY dst_addr, src_addr, count - Copy memory block
; Uses expressions to automatically split addresses into high/low bytes
; Clobbers: R0, R1, R2, R3, R6, R7
; Example:
;   SRC_ADDR = 0x4000
;   DST_ADDR = 0x5000
;   MEMCPY DST_ADDR, SRC_ADDR, 50    ; Copy 50 bytes: 0x4000 -> 0x5000
;   MEMCPY 0x5100, 0x4200, 100       ; Copy 100 bytes
; -----------------------------------------------------------------------------
.macro MEMCPY dst_addr, src_addr, count
    PUSH R2
    PUSH R3
    LOADI R2, src_addr >> 8
    LOADI R3, src_addr & 0xFF
    LOADI R6, dst_addr >> 8
    LOADI R7, dst_addr & 0xFF
    LOADI R1, count
@loop:
    LOAD R0, [R2:R3]+
    STORE R0, [R6:R7]+
    DEC R1
    JNZ @loop
    POP R3
    POP R2
.endmacro

; -----------------------------------------------------------------------------
; CLEAR_SCREEN color - Fill entire framebuffer with color
; Optimized loop fills all 16KB (128x128 pixels)
; Clobbers: R0, R1, R6, R7
; Example:
;   CLEAR_SCREEN 0x03    ; Dark blue background
;   CLEAR_SCREEN 0x00    ; Black screen
; -----------------------------------------------------------------------------
.macro CLEAR_SCREEN color
    LOADI R0, color
    LOADI R6, FRAMEBUFFER_HI
    LOADI R7, FRAMEBUFFER_LO
@loop:
    STORE R0, [R6:R7]
    INC R7
    JNZ @loop
    INC R6
    XOR R1, R1
    CMP R6, R1
    JNZ @loop
.endmacro

; =============================================================================
; 11. FRAMEBUFFER UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; FB_ADDR_FROM_XY - Compute framebuffer pointer from (x,y) in R0:R1
; Input: R0 = x (0-127), R1 = y (0-127)
; Output: R6:R7 = framebuffer address (0xC000 + y*128 + x)
; Formula optimized: y*128 = (y>>1)<<8 | (y<<7 & 0xFF)
; Clobbers: R3, R6, R7
; Example:
;   LOADI R0, 64    ; x = 64
;   LOADI R1, 64    ; y = 64
;   FB_ADDR_FROM_XY
;   ; R6:R7 now points to pixel at (64, 64)
;   LOADI R0, 0xFF
;   STORE R0, [R6:R7]    ; Draw white pixel
; -----------------------------------------------------------------------------
.macro FB_ADDR_FROM_XY
    ; Low byte: (y<<7) + x
    MOV R7, R1
    TIMES 7 SHL R7
    ADD R7, R0
    ; High byte: 0xC0 + (y>>1)
    MOV R6, R1
    SHR R6
    LOADI R3, FRAMEBUFFER_HI
    ADD R6, R3
.endmacro

; -----------------------------------------------------------------------------
; DRAW_PIXEL x_reg, y_reg, color - Draw single pixel
; Clobbers: R0, R1, R3, R6, R7
; Example:
;   LOADI R2, 10
;   LOADI R3, 20
;   DRAW_PIXEL R2, R3, 0xFF    ; White pixel at (10, 20)
; -----------------------------------------------------------------------------
.macro DRAW_PIXEL x_reg, y_reg, color
    MOV R0, x_reg
    MOV R1, y_reg
    ; Calculate framebuffer address in R6:R7
    ; Low byte: (y<<7) + x
    MOV R7, R1
    TIMES 7 SHL R7
    ADD R7, R0
    ; High byte: 0xC0 + (y>>1)
    MOV R6, R1
    SHR R6
    LOADI R3, FRAMEBUFFER_HI
    ADD R6, R3
    ; Store pixel
    LOADI R0, color
    STORE R0, [R6:R7]
.endmacro

; =============================================================================
; 12. DEBUGGING & DEVELOPMENT UTILITIES
; =============================================================================

; -----------------------------------------------------------------------------
; NOP - No operation (useful for debugging/patching)
; Clobbers: none
; Example:
;   NOP    ; Placeholder for future code
; -----------------------------------------------------------------------------
.macro NOP
    MOV R0, R0
.endmacro

; -----------------------------------------------------------------------------
; BREAKPOINT - Halt execution (debugging aid)
; Stops execution at this point
; Example:
;   BREAKPOINT    ; Stop here for debugging
; -----------------------------------------------------------------------------
.macro BREAKPOINT
    HALT
.endmacro

; =============================================================================
; COMMON PATTERNS & EXAMPLES
; =============================================================================
;
; EXPRESSION-BASED MACROS:
;   All macros use expressions to automatically split 16-bit addresses:
;
;   LOAD16 R0, 0x4010           ; Load from 0x4010
;   STORE16I 42, 0x4020         ; Store 42 to 0x4020
;   MEMSET 0x5000, 100, 0       ; Fill 100 bytes at 0x5000
;
;   With constants (recommended):
;   DATA_ADDR = 0x4010
;   BUFFER = 0x5000
;   LOAD16 R0, DATA_ADDR        ; Load from constant
;   STORE16I 42, DATA_ADDR + 16 ; Expressions work!
;   MEMSET BUFFER, 100, 0       ; Fill using constant
;
; =============================================================================

; BASIC GAME LOOP:
;   FRAME_VAR = 0x4000
;
;   section .data
;   ORG FRAME_VAR
;   last_frame: DB 0
;
;   section .code
;   START:
;       CLEAR_SCREEN 0x03
;   MAIN_LOOP:
;       WAIT_VSYNC FRAME_VAR
;       CALL UPDATE
;       CALL RENDER
;       JMP MAIN_LOOP
;
; READ INPUT:
;   READ_KEYS R0
;   LOADI R1, 0x01    ; UP key
;   AND R0, R1
;   JNZ HANDLE_UP
;
; DRAW SPRITE:
;   OAM_SET_SPRITE 0
;   LOADI R0, 64      ; x
;   LOADI R1, 64      ; y
;   OAM_WRITE_SPRITE R1, R0, 0, 0
;   PPU_SPRITES_ON
;
; 16-BIT COUNTER:
;   LOAD16I R0, R1, 0x00, 0x00
; LOOP:
;   INC16 R0, R1
;   LOAD16I R2, R3, 0x03, 0xE8    ; 1000
;   CMP16 R0, R1, R2, R3
;   JC LOOP
;
; CLAMPED VELOCITY:
;   LOAD16 R0, 0x40, 0x10    ; Load velocity
;   ADDI R0, 5               ; Accelerate
;   CLAMP_MAX R0, 50         ; Max speed
;
; =============================================================================
; END OF TINY16 STANDARD LIBRARY
; =============================================================================
