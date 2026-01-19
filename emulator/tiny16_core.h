#pragma once

#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>

#include "vm.h"

#define TINY16_EMU_PIXEL_WIDTH   128
#define TINY16_EMU_PIXEL_HEIGHT  128
#define TINY16_EMU_SCREEN_WIDTH  (TINY16_EMU_PIXEL_WIDTH * 8)
#define TINY16_EMU_SCREEN_HEIGHT (TINY16_EMU_PIXEL_HEIGHT * 8)
#define TINY16_EMU_TARGET_IPS    (60.0f * 1000) // 60k IPS = ~1000 instr/frame at 60 FPS

typedef struct {
    Tiny16VM* vm;
    Texture2D fb_texture;
    uint64_t frame_counter;
    float instr_acc;
    uint8_t back_buffer[TINY16_EMU_PIXEL_WIDTH * TINY16_EMU_PIXEL_HEIGHT];
    bool paused;
    bool program_loaded;
} Tiny16Emulator;

Tiny16Emulator* tiny16_emu_create(Tiny16VM* vm, bool program_loaded);
void tiny16_emu_destroy(Tiny16Emulator* emu);

void tiny16_emu_update_input(Tiny16VM* vm);
void tiny16_emu_update_texture(Texture2D* texture, const uint8_t* framebuffer);
void tiny16_emu_update_frame(Tiny16Emulator* emu);

bool tiny16_emu_run_cli(Tiny16VM* vm, uint32_t max_instructions);
