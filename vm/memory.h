#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TINY16_MEMORY_SIZE (1u << 16)

typedef struct {
    uint8_t bytes[TINY16_MEMORY_SIZE];
} Tiny16Memory;

#define TINY16_MEMORY_CODE_BEGIN  0x0010
#define TINY16_MEMORY_CODE_END    0x1FFF
#define TINY16_MEMORY_DATA_BEGIN  0x2000
#define TINY16_MEMORY_DATA_END    0x3FFF
#define TINY16_MEMORY_STACK_BEGIN 0x8000
#define TINY16_MEMORY_STACK_END   0xBEFF

// MMIO Control Registers

// Input (0xBF00-0xBF0F)
#define TINY16_MMIO_KEYS_STATE    0xBF00
#define TINY16_MMIO_KEYS_PRESSED  0xBF01
#define TINY16_MMIO_MOUSE_X       0xBF02
#define TINY16_MMIO_MOUSE_Y       0xBF03
#define TINY16_MMIO_MOUSE_BUTTONS 0xBF04

// Timers (0xBF20-0xBF2F)
#define TINY16_MMIO_TICK_LOW    0xBF20
#define TINY16_MMIO_TICK_HIGH   0xBF21
#define TINY16_MMIO_FRAME_COUNT 0xBF22
#define TINY16_MMIO_VSYNC       0xBF23

// Random (0xBF30-0xBF3F)

// System (0xBFF0-0xBFFF)

// Framebuffer (0xC000-0xFFFF)
#define TINY16_FRAMEBUFFER             0xC000
#define TINY16_FRAMEBUFFER_SIZE_WIDTH  128
#define TINY16_FRAMEBUFFER_SIZE_HEIGHT 128
#define TINY16_FRAMEBUFFER_ADDR(x, y)  (TINY16_FRAMEBUFFER + (y << 7u) + x)

void tiny16_memory_print(const Tiny16Memory* memory, bool framebuffer);
void tiny16_memory_reset(Tiny16Memory* memory);
size_t tiny16_memory_load_from_file(Tiny16Memory* memory, char* filename);
