#include <string.h>

#include "memory.h"
#include "ppu.h"

void tiny16_ppu_reset(Tiny16PPU* ppu) {
    ppu->ctrl = 0;
    ppu->scroll_x = 0;
    ppu->scroll_y = 0;
    ppu->status = 0;
    ppu->frame_counter = 0;
}

void tiny16_ppu_mmio_write(Tiny16PPU* ppu, uint16_t addr, uint8_t value) {
    switch (addr) {
    case TINY16_MMIO_PPU_CTRL:
        ppu->ctrl = value;
        break;
    case TINY16_MMIO_PPU_SCROLL_X:
        ppu->scroll_x = value;
        break;
    case TINY16_MMIO_PPU_SCROLL_Y:
        ppu->scroll_y = value;
        break;
    case TINY16_MMIO_PPU_STATUS:
        // Status is read-only from CPU side, but allow clearing VBLANK
        ppu->status &= ~value; // Writing 1 clears bits
        break;
    }
}

uint8_t tiny16_ppu_mmio_read(Tiny16PPU* ppu, uint16_t addr) {
    switch (addr) {
    case TINY16_MMIO_PPU_CTRL:
        return ppu->ctrl;
    case TINY16_MMIO_PPU_SCROLL_X:
        return ppu->scroll_x;
    case TINY16_MMIO_PPU_SCROLL_Y:
        return ppu->scroll_y;
    case TINY16_MMIO_PPU_STATUS: {
        uint8_t status = ppu->status;
        ppu->status &= ~TINY16_PPU_STATUS_VBLANK; // clear VBLANK
        return status;
    }
    }
    return 0;
}

static void tiny16_ppu_render_tile(uint8_t* framebuffer, const Tiny16Tile* tile,
                                   const Tiny16Palette* palette, int dst_x, int dst_y,
                                   uint8_t attr) {
    bool flip_h = attr & TINY16_OAM_ATTR_FLIP_H;
    bool flip_v = attr & TINY16_OAM_ATTR_FLIP_V;

    for (int ty = 0; ty < TINY16_TILE_HEIGHT; ++ty) {
        int src_y = flip_v ? (TINY16_TILE_HEIGHT - 1 - ty) : ty;
        for (int tx = 0; tx < TINY16_TILE_WIDTH; ++tx) {
            int src_x = flip_h ? (TINY16_TILE_WIDTH - 1 - tx) : tx;

            int px = dst_x + tx;
            int py = dst_y + ty;

            // clipping
            if (px < 0 || px >= TINY16_FRAMEBUFFER_SIZE_WIDTH) continue;
            if (py < 0 || py >= TINY16_FRAMEBUFFER_SIZE_HEIGHT) continue;

            // each row has 4 bytes (8 pixels at 4bpp)
            uint8_t byte = tile->rows[src_y][src_x / 2];
            uint8_t color_idx = (src_x & 1) ? (byte & 0x0F) : (byte >> 4);

            if (color_idx == 0) continue; // transparent

            framebuffer[py * TINY16_FRAMEBUFFER_SIZE_WIDTH + px] =
                palette->entries[color_idx].color;
        }
    }
}

static void tiny16_ppu_render_sprite(uint8_t* framebuffer, const Tiny16Tile* tiles,
                                     const Tiny16Palette* palette, const Tiny16OAMEntry* sprite) {
    if (sprite->y == TINY16_OAM_SPRITE_HIDDEN) return;
    tiny16_ppu_render_tile(framebuffer, &tiles[sprite->tile], palette, sprite->x, sprite->y,
                           sprite->attr);
}

void tiny16_ppu_render(Tiny16PPU* ppu, uint8_t* memory) {
    uint8_t* framebuffer = memory + TINY16_FRAMEBUFFER;
    const Tiny16Tile* tiles = (const Tiny16Tile*)(memory + TINY16_MEMORY_GRAPHICS_TILES_BASE);
    const Tiny16Tilemap* tilemap =
        (const Tiny16Tilemap*)(memory + TINY16_MEMORY_GRAPHICS_TILEMAP_BASE);
    const Tiny16Attrmap* attrmap =
        (const Tiny16Attrmap*)(memory + TINY16_MEMORY_GRAPHICS_ATTRMAP_BASE);
    const Tiny16Palette* palette =
        (const Tiny16Palette*)(memory + TINY16_MEMORY_GRAPHICS_PALETTE_BASE);

    // clear framebuffer to palette color 0 (background/transparent color)
    memset(framebuffer, palette->entries[0].color,
           TINY16_FRAMEBUFFER_SIZE_WIDTH * TINY16_FRAMEBUFFER_SIZE_HEIGHT);

    if (ppu->ctrl & TINY16_PPU_CTRL_ENABLE_BG) {
        for (int y = 0; y < TINY16_TILEMAP_HEIGHT; ++y) {
            for (int x = 0; x < TINY16_TILEMAP_WIDTH; ++x) {
                uint8_t tile_idx = tilemap->tiles[y][x];
                uint8_t attr = attrmap->attrs[y][x];

                int dst_x = (x * TINY16_TILE_WIDTH) - ppu->scroll_x;
                int dst_y = (y * TINY16_TILE_HEIGHT) - ppu->scroll_y;

                // wrap around for scrolling
                if (dst_x < -TINY16_TILE_WIDTH) dst_x += TINY16_TILEMAP_PIXEL_WIDTH;
                if (dst_y < -TINY16_TILE_HEIGHT) dst_y += TINY16_TILEMAP_PIXEL_HEIGHT;

                tiny16_ppu_render_tile(framebuffer, &tiles[tile_idx], palette, dst_x, dst_y, attr);
            }
        }
    }

    if (ppu->ctrl & TINY16_PPU_CTRL_ENABLE_SPRITES) {
        const Tiny16OAMEntry* oam =
            (const Tiny16OAMEntry*)(memory + TINY16_MEMORY_GRAPHICS_OAM_BASE);
        for (int i = 0; i < TINY16_OAM_SPRITE_COUNT; i++) {
            tiny16_ppu_render_sprite(framebuffer, tiles, palette, &oam[i]);
        }
    }

    // set VBLANK flag and increment frame counter
    ppu->status |= TINY16_PPU_STATUS_VBLANK;
    ppu->frame_counter++;

    // clear RENDER_NOW bit
    ppu->ctrl &= ~TINY16_PPU_CTRL_RENDER_NOW;
}
