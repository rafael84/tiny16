#pragma once

#include <stdint.h>

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
void tiny16_ppu_begin_frame(Tiny16PPU* ppu);
