#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "apu.h"
#include "cpu.h"
#include "memory.h"
#include "ppu.h"
#include "vm.h"

Tiny16VM* tiny16_vm_create(void) {
    Tiny16VM* vm = malloc(sizeof(Tiny16VM));
    tiny16_memory_reset(&vm->memory);
    tiny16_cpu_reset(&vm->cpu);
    tiny16_ppu_reset(&vm->ppu);
    tiny16_apu_reset(&vm->apu);
    vm->apu.memory = (struct Tiny16Memory*)&vm->memory;
    vm->ticks = 0;
    return vm;
}

void tiny16_vm_reset(Tiny16VM* vm) {
    tiny16_cpu_reset(&vm->cpu);
    tiny16_memory_reset(&vm->memory);
    tiny16_ppu_reset(&vm->ppu);
    tiny16_apu_reset(&vm->apu);
    vm->apu.memory = (struct Tiny16Memory*)&vm->memory;
    vm->ticks = 0;
}

bool tiny16_vm_step(Tiny16VM* vm) {
    tiny16_vm_mem_write(vm, TINY16_MMIO_TICK_LOW, vm->ticks & 0xFF);
    tiny16_vm_mem_write(vm, TINY16_MMIO_TICK_HIGH, (vm->ticks >> 8) & 0xFF);
    bool success = tiny16_cpu_step(&vm->cpu, vm, tiny16_vm_mem_read, tiny16_vm_mem_write);
    vm->ticks++;
    return success;
}

static inline bool tiny16_is_ppu_mmio(uint16_t addr) {
    return addr >= TINY16_MMIO_PPU_CTRL && addr <= TINY16_MMIO_PPU_STATUS;
}

static inline bool tiny16_is_apu_mmio(uint16_t addr) {
    // APU range: 0xBF40-0xBF5D (channels), 0xBF60-0xBF74 (music), 0xBF90-0xBF95 (SFX)
    return (addr >= TINY16_MMIO_APU_CTRL && addr <= TINY16_MMIO_APU_MUSIC_STATUS) ||
           (addr >= TINY16_MMIO_APU_SFX_PLAY && addr <= TINY16_MMIO_APU_SFX_COUNT);
}

uint8_t tiny16_vm_mem_read(void* ctx, uint16_t addr) {
    Tiny16VM* vm = ctx;
    if (tiny16_is_ppu_mmio(addr)) return tiny16_ppu_mmio_read(&vm->ppu, addr);
    if (tiny16_is_apu_mmio(addr)) return tiny16_apu_mmio_read(&vm->apu, addr);
    uint8_t value = vm->memory.bytes[addr];
    if (addr == TINY16_MMIO_KEYS_PRESSED) tiny16_vm_mem_write(vm, TINY16_MMIO_KEYS_PRESSED, 0);
    return value;
}

void tiny16_vm_mem_write(void* ctx, uint16_t addr, uint8_t value) {
    Tiny16VM* vm = ctx;

    if (addr >= TINY16_MEMORY_CODE_BEGIN && addr < TINY16_MEMORY_CODE_END) {
        fprintf(stderr, "[CRITICAL] MEM write to code segment: 0x%04X\n", addr);
        exit(EXIT_FAILURE);
    }

    if (tiny16_is_ppu_mmio(addr)) {
        tiny16_ppu_mmio_write(&vm->ppu, addr, value);
        if (addr == TINY16_MMIO_PPU_CTRL && (value & TINY16_PPU_CTRL_RENDER_NOW)) {
            tiny16_ppu_render(&vm->ppu, vm->memory.bytes);
        }
        return;
    }

    if (tiny16_is_apu_mmio(addr)) {
        tiny16_apu_mmio_write(&vm->apu, addr, value);
        return;
    }

    vm->memory.bytes[addr] = value;
}

bool tiny16_vm_exec(Tiny16VM* vm, uint64_t max_steps) {
    vm->ticks = 0;
    bool success;
    do {
        success = tiny16_vm_step(vm);
    } while ((vm->ticks < max_steps) && success);
    return vm->ticks <= max_steps;
}
