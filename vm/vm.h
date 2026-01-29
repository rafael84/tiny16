#pragma once

#include <stdint.h>

#include "apu.h"
#include "cpu.h"
#include "memory.h"
#include "ppu.h"

typedef struct {
    Tiny16CPU cpu;
    Tiny16Memory memory;
    Tiny16PPU ppu;
    Tiny16APU apu;
    uint32_t ticks;
} Tiny16VM;

Tiny16VM* tiny16_vm_create(void);
bool tiny16_vm_step(Tiny16VM* vm);
bool tiny16_vm_exec(Tiny16VM* vm, uint64_t max_steps);

uint8_t tiny16_vm_mem_read(void* ctx, uint16_t addr);
void tiny16_vm_mem_write(void* ctx, uint16_t addr, uint8_t value);

void tiny16_vm_reset(Tiny16VM* vm);
