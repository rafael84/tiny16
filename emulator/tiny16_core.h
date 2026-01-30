#pragma once

#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>

#include "vm.h"

#define TINY16_EMU_PIXEL_WIDTH   128
#define TINY16_EMU_PIXEL_HEIGHT  128
#define TINY16_EMU_SCREEN_WIDTH  (TINY16_EMU_PIXEL_WIDTH * 8)
#define TINY16_EMU_SCREEN_HEIGHT (TINY16_EMU_PIXEL_HEIGHT * 8)
#define TINY16_EMU_TARGET_IPS    (60.0f * 30000) // 1.8M IPS = ~30000 instr/frame at 60 FPS
#define TINY16_CPU_HZ            1800000u
#define TINY16_AUDIO_RING_FRAMES 16384u

typedef struct {
    Tiny16VM* vm;
    Texture2D fb_texture;
    AudioStream audio_stream;
    uint64_t frame_counter;
    float instr_acc;
    uint8_t back_buffer[TINY16_EMU_PIXEL_WIDTH * TINY16_EMU_PIXEL_HEIGHT];
    Color pixel_buffer[TINY16_EMU_PIXEL_WIDTH * TINY16_EMU_PIXEL_HEIGHT];
    Color rgb332_lut[256]; // lookup table for RGB332 -> RGBA conversion
    bool paused;
    bool program_loaded;
    bool audio_enabled;
    float audio_ring[TINY16_AUDIO_RING_FRAMES];
    volatile uint32_t audio_read;
    volatile uint32_t audio_write;
} Tiny16Emulator;

Tiny16Emulator* tiny16_emu_create(Tiny16VM* vm, bool program_loaded);
void tiny16_emu_destroy(Tiny16Emulator* emu);

void tiny16_emu_update_input(Tiny16VM* vm);
void tiny16_emu_update_texture(Tiny16Emulator* emu, const uint8_t* framebuffer);
void tiny16_emu_update_frame(Tiny16Emulator* emu);

bool tiny16_emu_run_cli(Tiny16VM* vm, uint32_t max_instructions);
