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
#include "vm.c"
#include "vm.h"

#define TINY16_EMU_PIXEL_WIDTH  TINY16_FRAMEBUFFER_SIZE_WIDTH
#define TINY16_EMU_PIXEL_HEIGHT TINY16_FRAMEBUFFER_SIZE_HEIGHT

#define TINY16_EMU_SCREEN_WIDTH  (TINY16_EMU_PIXEL_WIDTH * 8)
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

static args_t args;

int tiny16_emu_cli(Tiny16VM* vm);
int tiny16_emu_gui(Tiny16VM* vm);

int main(int argc, char** argv) {
    args = make_default_args();
    if (!parse_args(argc, argv, &args) || args.help || *args.program == '-') {
        print_help(argv[0]);
        return EXIT_FAILURE;
    }

    Tiny16VM* vm = tiny16_vm_create();

    if (!tiny16_memory_load_from_file(&vm->memory, args.program)) {
        fprintf(stderr, "could not load program from file\n");
        return EXIT_FAILURE;
    }

    tiny16_cpu_tracing = args.trace;
    if (tiny16_cpu_tracing) {
        puts("TRACING =========================================\n");
    }

    if (args.skip_instructions > 0) {
        vm->cpu.pc += args.skip_instructions * 3;
    }

    int result = 0;
    if (args.max_instructions > 0) {
        result = tiny16_emu_cli(vm);
    } else {
        result = tiny16_emu_gui(vm);
    }

    if (args.dump || args.dump_framebuffer) {
        puts("\nDUMP ===========================================\n");
        tiny16_cpu_print(&vm->cpu);
        tiny16_memory_print(&vm->memory, args.dump_framebuffer);
    }

    return result;
}

int tiny16_emu_cli(Tiny16VM* vm) {
    return tiny16_vm_exec(vm, args.max_instructions) ? EXIT_SUCCESS : EXIT_FAILURE;
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

void tiny16_emu_update_input(Tiny16VM* vm) {
    static uint8_t prev_keys = 0;
    uint8_t keys = 0;

    // keyboard: Down, Up, Left, Right, B, A, Start, Select
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) keys |= 0x80;  // bit 7
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) keys |= 0x40;    // bit 6
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) keys |= 0x20;  // bit 5
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) keys |= 0x10; // bit 4
    if (IsKeyDown(KEY_X) || IsKeyDown(KEY_K)) keys |= 0x08;     // bit 3
    if (IsKeyDown(KEY_C) || IsKeyDown(KEY_J)) keys |= 0x04;     // bit 2
    if (IsKeyDown(KEY_ENTER)) keys |= 0x02;                     // bit 1
    if (IsKeyDown(KEY_SPACE)) keys |= 0x01;                     // bit 0
    tiny16_vm_mem_write(vm, TINY16_MMIO_KEYS_STATE, keys);

    uint8_t pressed = keys & ~prev_keys; // edge detection
    uint8_t current_pressed =
        vm->memory.bytes[TINY16_MMIO_KEYS_PRESSED]; // direct read to avoid clear
    tiny16_vm_mem_write(vm, TINY16_MMIO_KEYS_PRESSED, current_pressed | pressed);
    prev_keys = keys;

    // mouse pos: clamp and scale to framebuffer coords (0-127)
    Vector2 mouse_pos = GetMousePosition();

    // Clamp to window bounds
    if (mouse_pos.x < 0) mouse_pos.x = 0;
    if (mouse_pos.x >= TINY16_EMU_SCREEN_WIDTH) mouse_pos.x = TINY16_EMU_SCREEN_WIDTH - 1;
    if (mouse_pos.y < 0) mouse_pos.y = 0;
    if (mouse_pos.y >= TINY16_EMU_SCREEN_HEIGHT) mouse_pos.y = TINY16_EMU_SCREEN_HEIGHT - 1;

    // Scale to framebuffer coordinates
    uint8_t mouse_x =
        (uint8_t)((mouse_pos.x / TINY16_EMU_SCREEN_WIDTH) * (TINY16_FRAMEBUFFER_SIZE_WIDTH - 1));
    uint8_t mouse_y =
        (uint8_t)((mouse_pos.y / TINY16_EMU_SCREEN_HEIGHT) * (TINY16_FRAMEBUFFER_SIZE_HEIGHT - 1));
    tiny16_vm_mem_write(vm, TINY16_MMIO_MOUSE_X, mouse_x);
    tiny16_vm_mem_write(vm, TINY16_MMIO_MOUSE_Y, mouse_y);

    // mouse buttons
    uint8_t mouse_buttons = 0;
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) mouse_buttons |= 0x1;
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) mouse_buttons |= 0x2;
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) mouse_buttons |= 0x4;
    tiny16_vm_mem_write(vm, TINY16_MMIO_MOUSE_BUTTONS, mouse_buttons);
}

int tiny16_emu_gui(Tiny16VM* vm) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(TINY16_EMU_SCREEN_WIDTH, TINY16_EMU_SCREEN_HEIGHT, "tiny16 emulator");
    // SetTargetFPS(60);

    Texture2D fb_texture =
        LoadTextureFromImage(GenImageColor(TINY16_EMU_PIXEL_WIDTH, TINY16_EMU_PIXEL_HEIGHT, BLACK));

    uint64_t frame_counter = 0u;
    float instr_acc = 0.0f;
    uint8_t back_buffer[TINY16_FRAMEBUFFER_SIZE_WIDTH * TINY16_FRAMEBUFFER_SIZE_HEIGHT] = {0};

    bool paused = false;
    while (!WindowShouldClose()) {

        if (IsKeyPressed(KEY_ESCAPE)) break;

        if (IsKeyPressed(KEY_P)) {
            paused = !paused;
        }

        if (!paused) {
            instr_acc += TINY16_EMU_TARGET_IPS * GetFrameTime();
            uint32_t instr_this_frame = (uint32_t)instr_acc;
            instr_acc -= instr_this_frame; // keep fractional part

            tiny16_vm_mem_write(vm, TINY16_MMIO_FRAME_COUNT, frame_counter & 0xFF);
            tiny16_vm_mem_write(vm, TINY16_MMIO_VSYNC, 0);

            tiny16_emu_update_input(vm);

            for (uint32_t step = 0; step < instr_this_frame; ++step) {
                if (tiny16_vm_mem_read(vm, TINY16_MMIO_VSYNC) == 1) {
                    memcpy(back_buffer, &vm->memory.bytes[TINY16_FRAMEBUFFER], sizeof(back_buffer));
                    tiny16_vm_mem_write(vm, TINY16_MMIO_VSYNC, 0);
                }
                if (!tiny16_vm_step(vm)) return EXIT_FAILURE;
            }
            frame_counter++;

            tiny16_emu_update_texture(&fb_texture, back_buffer);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(fb_texture,                                                           //
                       (Rectangle){0, 0, TINY16_EMU_PIXEL_WIDTH, TINY16_EMU_PIXEL_HEIGHT},   //
                       (Rectangle){0, 0, TINY16_EMU_SCREEN_WIDTH, TINY16_EMU_SCREEN_HEIGHT}, //
                       (Vector2){0, 0}, 0.0f, WHITE);
        DrawFPS(10, 10);
        EndDrawing();
    }

    UnloadTexture(fb_texture);
    CloseWindow();

    return EXIT_SUCCESS;
}
