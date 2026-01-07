#include <assert.h>
#include <raylib.h>
#include <stdbool.h>
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

#define TINY16_EMU_TARGET_IPS (60.0f * 100000)

// clang-format off
#define REQUIRED_ARGS \
    REQUIRED_STRING_ARG(program, "program", "Path to the tiny16 binary file")

#define OPTIONAL_ARGS \
    OPTIONAL_UINT_ARG(skip_instructions, 0, "-s", "skip_instructions", "Number of instructions to skip before emulation") \
    OPTIONAL_UINT_ARG(max_instructions, 0, "-m", "max_instructions", "Max number of instructions to run")

#define BOOLEAN_ARGS \
    BOOLEAN_ARG(dump, "-d", "Print CPU & Memory (no framebuffer) at the end of the emulation") \
    BOOLEAN_ARG(dump_framebuffer, "-df", "Print CPU & Memory at the end of the emulation") \
    BOOLEAN_ARG(trace, "-t", "Enable CPU tracing") \
    BOOLEAN_ARG(help, "-h", "Show help")

#include "easyargs.h"
// clang-format on

int tiny16_emu_cli(Tiny16CPU* cpu, Tiny16Memory* memory);
int tiny16_emu_gui(Tiny16CPU* cpu, Tiny16Memory* memory);

static args_t args;

int main(int argc, char** argv) {
    args = make_default_args();
    if (!parse_args(argc, argv, &args) || args.help || *args.program == '-') {
        print_help(argv[0]);
        return EXIT_FAILURE;
    }

    Tiny16Memory memory = {0};
    tiny16_memory_reset(&memory);
    if (!tiny16_memory_load_from_file(&memory, args.program)) {
        fprintf(stderr, "could not load program from file\n");
        return EXIT_FAILURE;
    }

    Tiny16CPU cpu = {0};
    tiny16_cpu_reset(&cpu);

    tiny16_cpu_tracing = args.trace;
    if (tiny16_cpu_tracing) {
        puts("TRACING =========================================\n");
    }

    if (args.skip_instructions > 0) {
        cpu.pc += args.skip_instructions * 3;
    }

    int result = 0;
    if (args.max_instructions > 0) {
        result = tiny16_emu_cli(&cpu, &memory);
    } else {
        result = tiny16_emu_gui(&cpu, &memory);
    }

    if (args.dump || args.dump_framebuffer) {
        puts("\nDUMP ===========================================\n");
        tiny16_cpu_print(&cpu);
        tiny16_memory_print(&memory, args.dump_framebuffer);
    }

    return result;
}

int tiny16_emu_cli(Tiny16CPU* cpu, Tiny16Memory* memory) {
    return tiny16_cpu_exec(cpu, memory, args.max_instructions) ? EXIT_SUCCESS : EXIT_FAILURE;
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

int tiny16_emu_gui(Tiny16CPU* cpu, Tiny16Memory* memory) {
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

            memory->bytes[TINY16_MMIO_FRAME_COUNT] = frame_counter & 0xFF;
            memory->bytes[TINY16_MMIO_VSYNC] = 0;

            for (uint32_t i = 0; i < instr_this_frame; ++i) {
                memory->bytes[TINY16_MMIO_TICK_LOW] = tick_counter & 0xFF;
                memory->bytes[TINY16_MMIO_TICK_HIGH] = (tick_counter >> 8) & 0xFF;

                if (memory->bytes[TINY16_MMIO_VSYNC] == 1) {
                    memcpy(back_buffer, &memory->bytes[TINY16_FRAMEBUFFER], sizeof(back_buffer));
                    memory->bytes[TINY16_MMIO_VSYNC] = 0;
                }

                if (!tiny16_cpu_step(cpu, memory)) {
                    return EXIT_FAILURE;
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

    return EXIT_SUCCESS;
}
