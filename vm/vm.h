#pragma once

#include <stdint.h>

#include "cpu.h"
#include "memory.h"

typedef struct {
    Tiny16CPU cpu;
    Tiny16Memory memory;
    uint32_t ticks;
} Tiny16VM;

Tiny16VM* tiny16_vm_create(void);
bool tiny16_vm_step(Tiny16VM* vm);

uint8_t tiny16_vm_memory_read(void* ctx, uint16_t addr);
void tiny16_vm_memory_write(void* ctx, uint16_t addr, uint8_t value);

void tiny16_vm_reset(Tiny16VM* vm);
