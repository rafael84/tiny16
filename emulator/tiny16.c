#include <assert.h>
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.c"
#include "cpu.h"
#include "memory.c"
#include "memory.h"

#define TINY16_EMU_PIXEL_WIDTH TINY16_FRAMEBUFFER_SIZE_WIDTH
#define TINY16_EMU_PIXEL_HEIGHT TINY16_FRAMEBUFFER_SIZE_HEIGHT

#define TINY16_EMU_SCREEN_WIDTH (TINY16_EMU_PIXEL_WIDTH * 8)
#define TINY16_EMU_SCREEN_HEIGHT (TINY16_EMU_PIXEL_HEIGHT * 8)

// #define TINY16_EMU_MAX_INSTRUCTIONS 300000
#define TINY16_EMU_TARGET_IPS (60.0f * 100000)

char* args_shift(int* argc, char*** argv);

void tiny16_emu_update_texture(Texture2D* texture, const uint8_t* framebuffer);

int main(int argc, char** argv) {
    char* program = args_shift(&argc, &argv);
    if (argc < 1) {
        fprintf(stderr, "Usage:\n\t%s program.tiny16\n", program);
    }

    char* filename = args_shift(&argc, &argv);

    Tiny16Memory memory = {0};
    tiny16_memory_reset(&memory);
    if (!tiny16_memory_load_from_file(&memory, filename)) {
        fprintf(stderr, "could not load program from file\n");
        exit(1);
    }

    Tiny16CPU cpu = {0};
    tiny16_cpu_reset(&cpu);

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(TINY16_EMU_SCREEN_WIDTH, TINY16_EMU_SCREEN_HEIGHT, "tiny16 emulator");
    SetTargetFPS(60);

    Texture2D fb_texture =
        LoadTextureFromImage(GenImageColor(TINY16_EMU_PIXEL_WIDTH, TINY16_EMU_PIXEL_HEIGHT, BLACK));

    uint64_t tick_counter = 0u;
    uint64_t frame_counter = 0u;
    float instr_acc = 0.0f;
    uint8_t back_buffer[TINY16_FRAMEBUFFER_SIZE_WIDTH * TINY16_FRAMEBUFFER_SIZE_HEIGHT] = {0};

    bool paused = false;
    while (!WindowShouldClose()) {

        if (IsKeyPressed(KEY_ESCAPE))
            break;

        if (IsKeyPressed(KEY_P)) {
            paused = !paused;
        }

        if (!paused) {
            instr_acc += TINY16_EMU_TARGET_IPS * GetFrameTime();
            uint32_t instr_this_frame = (uint32_t)instr_acc;
            instr_acc -= instr_this_frame; // keep fractional part

            memory.bytes[TINY16_MMIO_FRAME_COUNT] = frame_counter & 0xFF;
            memory.bytes[TINY16_MMIO_VSYNC] = 0;

            for (uint32_t i = 0; i < instr_this_frame; ++i) {
                memory.bytes[TINY16_MMIO_TICK_LOW] = tick_counter & 0xFF;
                memory.bytes[TINY16_MMIO_TICK_HIGH] = (tick_counter >> 8) & 0xFF;

                if (memory.bytes[TINY16_MMIO_VSYNC] == 1) {
                    memcpy(back_buffer, &memory.bytes[TINY16_FRAMEBUFFER], sizeof(back_buffer));
                    memory.bytes[TINY16_MMIO_VSYNC] = 0;
                }

                if (!tiny16_cpu_step(&cpu, &memory)) {
                    break;
                }
                tick_counter++;
            }
            frame_counter++;

            tiny16_emu_update_texture(&fb_texture, back_buffer);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(fb_texture,                                                           //
                       (Rectangle){0, 0, TINY16_EMU_PIXEL_WIDTH, TINY16_EMU_PIXEL_HEIGHT},   //
                       (Rectangle){0, 0, TINY16_EMU_SCREEN_WIDTH, TINY16_EMU_SCREEN_HEIGHT}, //
                       (Vector2){0, 0},                                                      //
                       0.0f,                                                                 //
                       WHITE                                                                 //
        );
        EndDrawing();
    }

    UnloadTexture(fb_texture);
    CloseWindow();

    return 0;
}

char* args_shift(int* argc, char*** argv) {
    assert(*argc > 0 && "argc <= 0");
    --(*argc);
    return *(*argv)++;
}

void tiny16_emu_update_texture(Texture2D* texture, const uint8_t* framebuffer) {
    uint16_t size = TINY16_EMU_PIXEL_WIDTH * TINY16_EMU_PIXEL_HEIGHT;
    Color pixels[size];
    for (int i = 0; i < size; ++i) {
        uint8_t value = framebuffer[i];
        // RGB332: RRRGGGBB
        uint8_t r = ((value >> 5) & 0x07) * 36; // 3 bits -> 0-252
        uint8_t g = ((value >> 2) & 0x07) * 36; // 3 bits -> 0-252
        uint8_t b = (value & 0x03) * 85;        // 2 bits -> 0-255
        pixels[i] = (Color){r, g, b, 255};
    }
    UpdateTexture(*texture, pixels);
}
