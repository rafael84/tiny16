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
    case TINY16_MMIO_PPU_CTRL: ppu->ctrl = value; break;
    case TINY16_MMIO_PPU_SCROLL_X: ppu->scroll_x = value; break;
    case TINY16_MMIO_PPU_SCROLL_Y: ppu->scroll_y = value; break;
    case TINY16_MMIO_PPU_STATUS: ppu->status &= ~value; break;
    }
}

uint8_t tiny16_ppu_mmio_read(Tiny16PPU* ppu, uint16_t addr) {
    switch (addr) {
    case TINY16_MMIO_PPU_CTRL: return ppu->ctrl;
    case TINY16_MMIO_PPU_SCROLL_X: return ppu->scroll_x;
    case TINY16_MMIO_PPU_SCROLL_Y: return ppu->scroll_y;
    case TINY16_MMIO_PPU_STATUS: {
        uint8_t status = ppu->status;
        ppu->status &= ~TINY16_PPU_STATUS_VBLANK;
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

            if (px < 0 || px >= TINY16_FRAMEBUFFER_SIZE_WIDTH) continue;
            if (py < 0 || py >= TINY16_FRAMEBUFFER_SIZE_HEIGHT) continue;

            // 4bpp: 2 pixels per byte
            // Each byte contains two 4-bit palette indices: [high nibble][low nibble]
            int byte_index = src_x / 2;
            uint8_t byte_data = tile->rows[src_y][byte_index];

            // Extract 4-bit palette index
            // Even pixels (0,2,4,6) use high nibble, odd pixels (1,3,5,7) use low nibble
            uint8_t palette_idx = (src_x & 1) ? (byte_data & 0x0F) : (byte_data >> 4);

            if (palette_idx == 0) continue; // transparent

            framebuffer[py * TINY16_FRAMEBUFFER_SIZE_WIDTH + px] =
                palette->entries[palette_idx].color;
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
    const Tiny16Tile* tiles = (const Tiny16Tile*)(memory + TINY16_MEMORY_GFX_TILES_BASE);
    const Tiny16Tilemap* tilemap = (const Tiny16Tilemap*)(memory + TINY16_MEMORY_GFX_TILEMAP_BASE);
    const Tiny16Palette* palette = (const Tiny16Palette*)(memory + TINY16_MEMORY_GFX_PALETTE_BASE);
    const Tiny16OAMEntry* oam = (const Tiny16OAMEntry*)(memory + TINY16_MEMORY_GFX_OAM_BASE);

    // clear framebuffer to palette color 0
    memset(framebuffer, palette->entries[0].color,
           TINY16_FRAMEBUFFER_SIZE_WIDTH * TINY16_FRAMEBUFFER_SIZE_HEIGHT);

    // Pass 1: Render sprites with BEHIND_BG flag (behind the background)
    if (ppu->ctrl & TINY16_PPU_CTRL_ENABLE_SPRITES) {
        for (int i = 0; i < TINY16_OAM_SPRITE_COUNT; i++) {
            if (oam[i].attr & TINY16_OAM_ATTR_BEHIND_BG) {
                tiny16_ppu_render_sprite(framebuffer, tiles, palette, &oam[i]);
            }
        }
    }

    // Pass 2: Render background (opaque pixels will cover "behind" sprites)
    if (ppu->ctrl & TINY16_PPU_CTRL_ENABLE_BG) {
        for (int y = 0; y < TINY16_TILEMAP_HEIGHT; ++y) {
            for (int x = 0; x < TINY16_TILEMAP_WIDTH; ++x) {
                const Tiny16TilemapEntry* entry = &tilemap->entries[y][x];
                uint8_t tile_idx = entry->tile;
                uint8_t attr = entry->attr;

                int dst_x = (x * TINY16_TILE_WIDTH) - ppu->scroll_x;
                int dst_y = (y * TINY16_TILE_HEIGHT) - ppu->scroll_y;

                // wrap around for scrolling
                if (dst_x < -TINY16_TILE_WIDTH) dst_x += TINY16_TILEMAP_PIXEL_WIDTH;
                if (dst_y < -TINY16_TILE_HEIGHT) dst_y += TINY16_TILEMAP_PIXEL_HEIGHT;

                tiny16_ppu_render_tile(framebuffer, &tiles[tile_idx], palette, dst_x, dst_y, attr);
            }
        }
    }

    // Pass 3: Render normal sprites (on top of background)
    if (ppu->ctrl & TINY16_PPU_CTRL_ENABLE_SPRITES) {
        for (int i = 0; i < TINY16_OAM_SPRITE_COUNT; i++) {
            if (!(oam[i].attr & TINY16_OAM_ATTR_BEHIND_BG)) {
                tiny16_ppu_render_sprite(framebuffer, tiles, palette, &oam[i]);
            }
        }
    }

    // set VBLANK flag and increment frame counter
    ppu->status |= TINY16_PPU_STATUS_VBLANK;
    ppu->frame_counter++;

    // clear RENDER_NOW bit
    ppu->ctrl &= ~TINY16_PPU_CTRL_RENDER_NOW;
}
