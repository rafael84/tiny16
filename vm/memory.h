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
// Graphics (PPU-visible, expanded layout)
//
#define TINY16_MEMORY_GFX_TILES_BASE 0x5000
#define TINY16_MEMORY_GFX_TILES_END  0x6FFF // TILES, 256 x 32 B = 8 KB

#define TINY16_MEMORY_GFX_TILEMAP_BASE 0x7000
#define TINY16_MEMORY_GFX_TILEMAP_END  0x8FFF // TILEMAP, 128 x 32 x 2 B = 8 KB

#define TINY16_MEMORY_GFX_OAM_BASE 0x9000
#define TINY16_MEMORY_GFX_OAM_END  0x91FF // OAM, 128 sprites x 4 B = 512 B

#define TINY16_MEMORY_GFX_PALETTE_BASE 0x9200
#define TINY16_MEMORY_GFX_PALETTE_END  0x923F // PALETTE, 4 x 16 colors = 64 B

//
// Stack
//
#define TINY16_MEMORY_STACK_BEGIN 0xA000
#define TINY16_MEMORY_STACK_END   0xBEFF // STACK, ~7.7 KB

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

// APU (0xBF40 - 0xBF5F)
#define TINY16_MMIO_APU_CTRL   0xBF40 // Master control: bit 0=enable, bits 4-7=volume
#define TINY16_MMIO_APU_STATUS 0xBF41 // Status: bits 0-4=channel active flags

// APU Channel 0 - Pulse 1
#define TINY16_MMIO_APU_CH0_FREQ_LO 0xBF42 // Frequency low byte
#define TINY16_MMIO_APU_CH0_FREQ_HI 0xBF43 // Frequency high (bits 0-2)
#define TINY16_MMIO_APU_CH0_VOL     0xBF44 // Volume (0-3) + duty (4-5)
#define TINY16_MMIO_APU_CH0_CTRL    0xBF45 // Enable (0) + trigger (1)

// APU Channel 1 - Pulse 2
#define TINY16_MMIO_APU_CH1_FREQ_LO 0xBF46
#define TINY16_MMIO_APU_CH1_FREQ_HI 0xBF47
#define TINY16_MMIO_APU_CH1_VOL     0xBF48
#define TINY16_MMIO_APU_CH1_CTRL    0xBF49

// APU Channel 2 - Triangle
#define TINY16_MMIO_APU_CH2_FREQ_LO 0xBF4A
#define TINY16_MMIO_APU_CH2_FREQ_HI 0xBF4B
#define TINY16_MMIO_APU_CH2_VOL     0xBF4C
#define TINY16_MMIO_APU_CH2_CTRL    0xBF4D

// APU Channel 3 - Noise
#define TINY16_MMIO_APU_CH3_PERIOD 0xBF4E // Period (0-15)
#define TINY16_MMIO_APU_CH3_VOL    0xBF4F // Volume (0-15)
#define TINY16_MMIO_APU_CH3_CTRL   0xBF50 // Enable (0) + trigger (1) + mode (2)

// APU Envelopes (AD = attack/decay, SR = sustain/release)
#define TINY16_MMIO_APU_CH0_ENV_AD 0xBF51 // Attack (hi nibble) / Decay (lo nibble)
#define TINY16_MMIO_APU_CH0_ENV_SR 0xBF52 // Sustain (hi nibble) / Release (lo nibble)
#define TINY16_MMIO_APU_CH1_ENV_AD 0xBF53
#define TINY16_MMIO_APU_CH1_ENV_SR 0xBF54
#define TINY16_MMIO_APU_CH2_ENV_AD 0xBF55
#define TINY16_MMIO_APU_CH2_ENV_SR 0xBF56
#define TINY16_MMIO_APU_CH3_ENV_AD 0xBF57
#define TINY16_MMIO_APU_CH3_ENV_SR 0xBF58

// APU Sweep + Length
#define TINY16_MMIO_APU_CH0_SWEEP 0xBF59 // Sweep rate (hi nibble), dir (bit3), shift (bits 0-2)
#define TINY16_MMIO_APU_CH1_SWEEP 0xBF5A
#define TINY16_MMIO_APU_CH0_LEN   0xBF5B // Length in frames (60 Hz)
#define TINY16_MMIO_APU_CH1_LEN   0xBF5C
#define TINY16_MMIO_APU_CH2_LEN   0xBF5D
#define TINY16_MMIO_APU_CH3_LEN   0xBF5E

// APU Wave Channel
#define TINY16_MMIO_APU_WAVE_FREQ_LO 0xBF60
#define TINY16_MMIO_APU_WAVE_FREQ_HI 0xBF61
#define TINY16_MMIO_APU_WAVE_VOL     0xBF62
#define TINY16_MMIO_APU_WAVE_CTRL    0xBF63 // Enable + trigger
#define TINY16_MMIO_APU_WAVE_LEN     0xBF64
#define TINY16_MMIO_APU_WAVE_ENV_AD  0xBF65
#define TINY16_MMIO_APU_WAVE_ENV_SR  0xBF66

// Wave RAM (32 x 4-bit samples)
#define TINY16_MMIO_APU_WAVE_RAM 0xBF70 // 0xBF70 - 0xBF8F

// RESERVED (0xBFF0 - 0xBFFF)

//
// Data section range: covers user data + all graphics memory
//
#define TINY16_DATA_BEGIN TINY16_MEMORY_DATA_BEGIN      // 0x4000
#define TINY16_DATA_END   TINY16_MEMORY_GFX_PALETTE_END // 0x923F

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
size_t tiny16_memory_load_from_file(Tiny16Memory* memory, const char* filename);

//
// Bus interface (used by CPU)
//
typedef uint8_t (*tiny16_mem_read_fn)(void* ctx, uint16_t addr);
typedef void (*tiny16_mem_write_fn)(void* ctx, uint16_t addr, uint8_t value);
