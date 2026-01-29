; vim: set ft=asm:
; =============================================================================
; TINY16 HARDWARE CONSTANTS
; =============================================================================
; MMIO addresses, graphics memory, key definitions
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

; PPU control (read/write)
PPU_CTRL_ADDR       = MMIO_BASE + 0x30  ; Graphics control register
PPU_SCROLL_X_ADDR   = MMIO_BASE + 0x31  ; Horizontal scroll (0-255)
PPU_SCROLL_Y_ADDR   = MMIO_BASE + 0x32  ; Vertical scroll (0-255)
PPU_STATUS_ADDR     = MMIO_BASE + 0x33  ; PPU status register

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
PPU_SCROLL_X_HI     = PPU_SCROLL_X_ADDR >> 8
PPU_SCROLL_X_LO     = PPU_SCROLL_X_ADDR & 0xFF
PPU_SCROLL_Y_HI     = PPU_SCROLL_Y_ADDR >> 8
PPU_SCROLL_Y_LO     = PPU_SCROLL_Y_ADDR & 0xFF
PPU_STATUS_HI       = PPU_STATUS_ADDR >> 8
PPU_STATUS_LO       = PPU_STATUS_ADDR & 0xFF

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

; Key bit definitions
KEY_SELECT          = 0x01              ; bit 0
KEY_START           = 0x02              ; bit 1
KEY_A               = 0x04              ; bit 2
KEY_B               = 0x08              ; bit 3
KEY_RIGHT           = 0x10              ; bit 4
KEY_LEFT            = 0x20              ; bit 5
KEY_UP              = 0x40              ; bit 6
KEY_DOWN            = 0x80              ; bit 7
