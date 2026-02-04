#pragma once

#include <stdint.h>

// Tile (4bpp - 2 pixels per byte)
#define TINY16_TILE_WIDTH     8
#define TINY16_TILE_HEIGHT    8
#define TINY16_TILE_BPP       4
#define TINY16_TILE_ROW_BYTES 4
#define TINY16_TILE_SIZE      32
#define TINY16_TILE_COUNT     255

typedef struct {
    uint8_t rows[TINY16_TILE_HEIGHT][TINY16_TILE_ROW_BYTES];
} Tiny16Tile;

// Tilemap (combined tile + attributes)
#define TINY16_TILEMAP_WIDTH        128
#define TINY16_TILEMAP_HEIGHT       32
#define TINY16_TILEMAP_PIXEL_WIDTH  (TINY16_TILEMAP_WIDTH * TINY16_TILE_WIDTH)
#define TINY16_TILEMAP_PIXEL_HEIGHT (TINY16_TILEMAP_HEIGHT * TINY16_TILE_HEIGHT)

// Tilemap entry attributes
enum {
    TINY16_TILEMAP_ATTR_FLIP_H = 1 << 0,
    TINY16_TILEMAP_ATTR_FLIP_V = 1 << 1,
    TINY16_TILEMAP_ATTR_PALETTE_SHIFT = 2,
    TINY16_TILEMAP_ATTR_PALETTE_MASK = 0x0C,
};

// Combined tilemap entry: 2 bytes per tile
typedef struct {
    uint8_t tile;
    uint8_t attr;
} Tiny16TilemapEntry;

typedef struct {
    Tiny16TilemapEntry entries[TINY16_TILEMAP_HEIGHT][TINY16_TILEMAP_WIDTH];
} Tiny16Tilemap;

// Palette
#define TINY16_PALETTE_SIZE  16
#define TINY16_PALETTE_COUNT 4

typedef struct {
    uint8_t color;
} Tiny16PaletteEntry;

typedef struct {
    Tiny16PaletteEntry entries[TINY16_PALETTE_SIZE];
} Tiny16Palette;

// OAM
enum {
    TINY16_OAM_ATTR_PALETTE_MASK = 0x03,
    TINY16_OAM_ATTR_BEHIND_BG = 1 << 5,
    TINY16_OAM_ATTR_FLIP_H = 1 << 6,
    TINY16_OAM_ATTR_FLIP_V = 1 << 7,
};

#define TINY16_OAM_SPRITE_COUNT  128
#define TINY16_OAM_SPRITE_HIDDEN 0xFF

typedef struct {
    uint8_t y;
    uint8_t x;
    uint8_t tile;
    uint8_t attr;
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
