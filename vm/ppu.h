#pragma once

#include <stdint.h>

// Tile (4bpp - 2 pixels per byte)
#define TINY16_TILE_WIDTH     8
#define TINY16_TILE_HEIGHT    8
#define TINY16_TILE_BPP       4   // bits per pixel
#define TINY16_TILE_ROW_BYTES 4   // 8 pixels * 4bpp / 8 = 4 bytes per row
#define TINY16_TILE_SIZE      32  // 8 rows * 4 bytes = 32 bytes per tile
#define TINY16_TILE_COUNT     255 // 255 tiles * 32 bytes = 8160 bytes (~8 KB)

typedef struct {
    uint8_t rows[TINY16_TILE_HEIGHT][TINY16_TILE_ROW_BYTES];
} Tiny16Tile;

// Tilemap (combined tile + attributes)
#define TINY16_TILEMAP_WIDTH        128
#define TINY16_TILEMAP_HEIGHT       32
#define TINY16_TILEMAP_PIXEL_WIDTH  (TINY16_TILEMAP_WIDTH * TINY16_TILE_WIDTH)   // 1024
#define TINY16_TILEMAP_PIXEL_HEIGHT (TINY16_TILEMAP_HEIGHT * TINY16_TILE_HEIGHT) // 256

// Tilemap entry attributes
enum {
    TINY16_TILEMAP_ATTR_FLIP_H = 1 << 0,
    TINY16_TILEMAP_ATTR_FLIP_V = 1 << 1,
    TINY16_TILEMAP_ATTR_PALETTE_SHIFT = 2,
    TINY16_TILEMAP_ATTR_PALETTE_MASK = 0x0C, // bits 2-3: palette index (0-3)
};

// Combined tilemap entry: 2 bytes per tile
typedef struct {
    uint8_t tile; // Tile index (0-255)
    uint8_t attr; // Attributes: [------VH] V=vflip, H=hflip
} Tiny16TilemapEntry;

typedef struct {
    Tiny16TilemapEntry entries[TINY16_TILEMAP_HEIGHT][TINY16_TILEMAP_WIDTH];
} Tiny16Tilemap;

// Palette
#define TINY16_PALETTE_SIZE  16
#define TINY16_PALETTE_COUNT 4

typedef struct {
    uint8_t color; // RGB332: RRRGGGBB
} Tiny16PaletteEntry;

typedef struct {
    Tiny16PaletteEntry entries[TINY16_PALETTE_SIZE];
} Tiny16Palette;

// OAM
enum {
    TINY16_OAM_ATTR_PALETTE_MASK = 0x03, // bits 0-1: palette index (0-3)
    TINY16_OAM_ATTR_BEHIND_BG = 1 << 5,  // Render behind opaque background pixels
    TINY16_OAM_ATTR_FLIP_H = 1 << 6,
    TINY16_OAM_ATTR_FLIP_V = 1 << 7,
};

#define TINY16_OAM_SPRITE_COUNT  128
#define TINY16_OAM_SPRITE_HIDDEN 0xFF

typedef struct {
    uint8_t y;    // Y position (0xFF = hidden)
    uint8_t x;    // X position
    uint8_t tile; // Tile index (0-255)
    uint8_t attr; // Attributes: [VHB__PPP] V=vflip, H=hflip, B=behind BG, P=palette (0-3)
} Tiny16OAMEntry;

// PPU
enum {
    TINY16_PPU_CTRL_ENABLE_BG = 1 << 0,
    TINY16_PPU_CTRL_ENABLE_SPRITES = 1 << 1,
    TINY16_PPU_CTRL_RENDER_NOW = 1 << 7,
};

enum {
    TINY16_PPU_STATUS_VBLANK = 1 << 0,
};

typedef struct {
    uint8_t ctrl;
    uint8_t scroll_x;
    uint8_t scroll_y;
    uint8_t status;
    uint32_t frame_counter;
} Tiny16PPU;

void tiny16_ppu_reset(Tiny16PPU* ppu);
void tiny16_ppu_mmio_write(Tiny16PPU* ppu, uint16_t addr, uint8_t value);
uint8_t tiny16_ppu_mmio_read(Tiny16PPU* ppu, uint16_t addr);
void tiny16_ppu_render(Tiny16PPU* ppu, uint8_t* memory);
