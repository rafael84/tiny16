#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MEMORY_SIZE (1u << 16)

typedef struct {
    uint8_t bytes[MEMORY_SIZE];
} Memory;

#define MEMORY_CODE_BEGIN 0x0000
#define MEMORY_CODE_END 0x1FFF

#define MEMORY_DATA_BEGIN 0x2000
#define MEMORY_DATA_END 0x3FFF

#define MEMORY_STACK_BEGIN 0x8000
#define MEMORY_STACK_END 0xBFFF

#define MMIO_FRAMEBUFFER 0xC000

#ifdef TINY16_DEVELOPMENT_MODE
#define MMIO_FRAMEBUFFER_SIZE_WIDTH 16
#define MMIO_FRAMEBUFFER_SIZE_HEIGHT 16
#define MMIO_FRAMEBUFFER_ADDR(x, y) (MMIO_FRAMEBUFFER + (y * MMIO_FRAMEBUFFER_SIZE_WIDTH) + x)
#else
#define MMIO_FRAMEBUFFER_SIZE_WIDTH 128
#define MMIO_FRAMEBUFFER_SIZE_HEIGHT 128
#define MMIO_FRAMEBUFFER_ADDR(x, y) (MMIO_FRAMEBUFFER + (y << 7u) + x)
#endif

#define MMIO_INPUT_KEY_STATE 0xE000

#define MMIO_TIMER_TICKS_LOW 0xE200
#define MMIO_TIMER_TICKS_HIGH 0xE201

void memory_print(const Memory* memory, bool framebuffer);
void memory_reset(Memory* memory);
size_t memory_load_from_file(Memory* memory, char* filename);
