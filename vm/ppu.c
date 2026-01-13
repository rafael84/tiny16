#include "ppu.h"

void tiny16_ppu_reset(Tiny16PPU* ppu) {
    ppu->ctrl = 0;
    ppu->scroll_x = 0;
    ppu->scroll_y = 0;
    ppu->status = 0;
    ppu->frame_counter = 0;
}

void tiny16_ppu_mmio_write(Tiny16PPU* ppu, uint16_t addr, uint8_t value) {
    (void)ppu;
    (void)addr;
    (void)value;
}

uint8_t tiny16_ppu_mmio_read(Tiny16PPU* ppu, uint16_t addr) {
    (void)ppu;
    (void)addr;
    return 0;
}

void tiny16_ppu_begin_frame(Tiny16PPU* ppu) {
    //
    (void)ppu;
}
