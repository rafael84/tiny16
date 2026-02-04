; vim: set ft=asm:
; =============================================================================
; TINY16 HARDWARE CONSTANTS
; =============================================================================
; MMIO addresses, graphics memory, key definitions
; =============================================================================

MMIO_BASE           = 0xBF00
MMIO_HI             = MMIO_BASE >> 8

; Input devices (read-only)
KEYS_STATE_ADDR     = MMIO_BASE + 0x00
KEYS_PRESSED_ADDR   = MMIO_BASE + 0x01
MOUSE_X_ADDR        = MMIO_BASE + 0x02
MOUSE_Y_ADDR        = MMIO_BASE + 0x03
MOUSE_BUTTONS_ADDR  = MMIO_BASE + 0x04

; Display timing (read-only)
FRAME_COUNT_ADDR    = MMIO_BASE + 0x22

; PPU control (read/write)
PPU_CTRL_ADDR       = MMIO_BASE + 0x30
PPU_SCROLL_X_ADDR   = MMIO_BASE + 0x31
PPU_SCROLL_Y_ADDR   = MMIO_BASE + 0x32
PPU_STATUS_ADDR     = MMIO_BASE + 0x33

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
FRAMEBUFFER_BASE    = 0xC000
FRAMEBUFFER_HI      = FRAMEBUFFER_BASE >> 8
FRAMEBUFFER_LO      = FRAMEBUFFER_BASE & 0xFF

GFX_TILES_BASE      = 0x5000
GFX_OAM_BASE        = 0x7800
GFX_PALETTE_BASE    = 0x7900

GFX_OAM_SPRITE_COUNT = 64
GFX_OAM_ENTRY_SIZE   = 4

; PPU control bits
PPU_CTRL_SPRITES    = 0x02
PPU_CTRL_RENDER_NOW = 0x80
PPU_CTRL_SPRITES_ON = PPU_CTRL_SPRITES | PPU_CTRL_RENDER_NOW

; Key bit definitions
KEY_SELECT          = 0x01
KEY_START           = 0x02
KEY_A               = 0x04
KEY_B               = 0x08
KEY_RIGHT           = 0x10
KEY_LEFT            = 0x20
KEY_UP              = 0x40
KEY_DOWN            = 0x80
