#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TINY16_MEMORY_SIZE (1u << 16)

typedef struct {
    uint8_t bytes[TINY16_MEMORY_SIZE];
} Tiny16Memory;

#define TINY16_MEMORY_CODE_BEGIN 0x0010
#define TINY16_MEMORY_CODE_END 0x1FFF

#define TINY16_MEMORY_DATA_BEGIN 0x2000
#define TINY16_MEMORY_DATA_END 0x3FFF

#define TINY16_MEMORY_STACK_BEGIN 0x8000
#define TINY16_MEMORY_STACK_END 0xBFFF

#define TINY16_MMIO_FRAMEBUFFER 0xC000
#define TINY16_MMIO_FRAMEBUFFER_SIZE_WIDTH 128
#define TINY16_MMIO_FRAMEBUFFER_SIZE_HEIGHT 128
#define TINY16_MMIO_FRAMEBUFFER_ADDR(x, y) (TINY16_MMIO_FRAMEBUFFER + (y << 7u) + x)

#define TINY16_MMIO_INPUT_KEY_STATE 0xE000

#define TINY16_MMIO_TIMER_TICKS_LOW 0xE200
#define TINY16_MMIO_TIMER_TICKS_HIGH 0xE201

void tiny16_memory_print(const Tiny16Memory* memory, bool framebuffer);
void tiny16_memory_reset(Tiny16Memory* memory);
size_t tiny16_memory_load_from_file(Tiny16Memory* memory, char* filename);
