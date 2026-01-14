#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TINY16_MEMORY_SIZE (1u << 16)

typedef struct {
    uint8_t bytes[TINY16_MEMORY_SIZE];
} Tiny16Memory;

//
// Signature (0x0000 - 0x000F)
//

//
// Code
//
#define TINY16_MEMORY_CODE_BEGIN 0x0010
#define TINY16_MEMORY_CODE_END   0x3FFF /* CODE, ~16 KB */

//
// User Data (contiguous, arbitrary)
//
#define TINY16_MEMORY_DATA_BEGIN 0x4000
#define TINY16_MEMORY_DATA_END   0x4FFF /* USER DATA, 4 KB */

//
// Graphics (PPU-visible, contiguous)
//
#define TINY16_MEMORY_GRAPHICS_TILES_BASE 0x5000
#define TINY16_MEMORY_GRAPHICS_TILES_END  0x6FFF /* TILES, 256 x 32 B = 8 KB */

#define TINY16_MEMORY_GRAPHICS_TILEMAP_BASE 0x7000
#define TINY16_MEMORY_GRAPHICS_TILEMAP_END  0x73FF /* TILEMAP, 32 x 32 = 1 KB */

#define TINY16_MEMORY_GRAPHICS_ATTRMAP_BASE 0x7400
#define TINY16_MEMORY_GRAPHICS_ATTRMAP_END  0x77FF /* ATTRMAP, 32 x 32 = 1 KB */

#define TINY16_MEMORY_GRAPHICS_OAM_BASE 0x7800
#define TINY16_MEMORY_GRAPHICS_OAM_END  0x78FF /* OAM, 64 sprites x 4 B = 256 B */

#define TINY16_MEMORY_GRAPHICS_PALETTE_BASE 0x7900
#define TINY16_MEMORY_GRAPHICS_PALETTE_END  0x791F /* PALETTE, 16 colors x 2 B = 32 B */

//
// Stack
//
#define TINY16_MEMORY_STACK_BEGIN 0x8000
#define TINY16_MEMORY_STACK_END   0xBEFF /* STACK, ~16 KB */

//
// MMIO
//

// Input
#define TINY16_MMIO_KEYS_STATE    0xBF00
#define TINY16_MMIO_KEYS_PRESSED  0xBF01
#define TINY16_MMIO_MOUSE_X       0xBF02
#define TINY16_MMIO_MOUSE_Y       0xBF03
#define TINY16_MMIO_MOUSE_BUTTONS 0xBF04

// Timers / System
#define TINY16_MMIO_TICK_LOW    0xBF20
#define TINY16_MMIO_TICK_HIGH   0xBF21
#define TINY16_MMIO_FRAME_COUNT 0xBF22

// PPU
#define TINY16_MMIO_PPU_CTRL     0xBF30
#define TINY16_MMIO_PPU_SCROLL_X 0xBF31
#define TINY16_MMIO_PPU_SCROLL_Y 0xBF32
#define TINY16_MMIO_PPU_STATUS   0xBF33

// RESERVED (0xBFF0 - 0xBFFF)

//
// Data section range: covers user data + all graphics memory
//
#define TINY16_DATA_BEGIN TINY16_MEMORY_DATA_BEGIN           // 0x4000
#define TINY16_DATA_END   TINY16_MEMORY_GRAPHICS_PALETTE_END // 0x791F

//
// Framebuffer (direct bitmap)
//
#define TINY16_FRAMEBUFFER             0xC000
#define TINY16_FRAMEBUFFER_SIZE_WIDTH  128
#define TINY16_FRAMEBUFFER_SIZE_HEIGHT 128

#define TINY16_FRAMEBUFFER_ADDR(x, y) (TINY16_FRAMEBUFFER + ((y) << 7u) + (x))

//
// Memory helpers
//
void tiny16_memory_print(const Tiny16Memory* memory, bool framebuffer);
void tiny16_memory_reset(Tiny16Memory* memory);
size_t tiny16_memory_load_from_file(Tiny16Memory* memory, char* filename);

//
// Bus interface (used by CPU)
//
typedef uint8_t (*tiny16_mem_read_fn)(void* ctx, uint16_t addr);
typedef void (*tiny16_mem_write_fn)(void* ctx, uint16_t addr, uint8_t value);
