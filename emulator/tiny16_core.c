#include "tiny16_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Tiny16Emulator* tiny16_emu_create(Tiny16VM* vm, bool program_loaded) {
    Tiny16Emulator* emu = malloc(sizeof(Tiny16Emulator));
    emu->vm = vm;
    emu->frame_counter = 0;
    emu->instr_acc = 0.0f;
    emu->paused = false;
    emu->program_loaded = program_loaded;
    memset(emu->back_buffer, 0, sizeof(emu->back_buffer));

    // Create texture
    emu->fb_texture =
        LoadTextureFromImage(GenImageColor(TINY16_EMU_PIXEL_WIDTH, TINY16_EMU_PIXEL_HEIGHT, BLACK));

    return emu;
}

void tiny16_emu_destroy(Tiny16Emulator* emu) {
    if (emu) {
        UnloadTexture(emu->fb_texture);
        free(emu);
    }
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

void tiny16_emu_update_frame(Tiny16Emulator* emu) {
    Tiny16VM* vm = emu->vm;

    // Handle pause toggle
    if (IsKeyPressed(KEY_P)) emu->paused = !emu->paused;

    // Handle debug dump
    if (IsKeyPressed(KEY_ZERO)) {
        tiny16_cpu_print(&vm->cpu);
        tiny16_memory_print(&vm->memory, true);
    }

    // Skip execution if no program is loaded
    if (!emu->program_loaded) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("No program loaded", 10, 10, 20, GRAY);
        DrawText("Use file input to load a .tiny16 file", 10, 40, 20, GRAY);
        EndDrawing();
        return;
    }

    if (!emu->paused) {
        float delta = GetFrameTime();
        if (delta > 0.1f) delta = 0.1f; // cap delta to 100ms to avoid spiral
        emu->instr_acc += TINY16_EMU_TARGET_IPS * delta;

        // cap instructions per frame to avoid runaway on lag
        uint32_t max_instr = (uint32_t)(TINY16_EMU_TARGET_IPS / 30.0f); // max ~2 frames worth
        uint32_t instr_this_frame = (uint32_t)emu->instr_acc;
        if (instr_this_frame > max_instr) {
            instr_this_frame = max_instr;
            emu->instr_acc = 0.0f; // reset accumulator on cap
        } else {
            emu->instr_acc -= instr_this_frame;
        }

        tiny16_vm_mem_write(vm, TINY16_MMIO_FRAME_COUNT, emu->frame_counter & 0xFF);

        tiny16_emu_update_input(vm);

        for (uint32_t step = 0; step < instr_this_frame; ++step) {
            if (!tiny16_vm_step(vm)) {
                emu->paused = true;
                break;
            }
        }
        emu->frame_counter++;

        // always update texture from framebuffer (PPU renders synchronously)
        memcpy(emu->back_buffer, &vm->memory.bytes[TINY16_FRAMEBUFFER], sizeof(emu->back_buffer));
        tiny16_emu_update_texture(&emu->fb_texture, emu->back_buffer);
    }

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(emu->fb_texture,                                                      //
                   (Rectangle){0, 0, TINY16_EMU_PIXEL_WIDTH, TINY16_EMU_PIXEL_HEIGHT},   //
                   (Rectangle){0, 0, TINY16_EMU_SCREEN_WIDTH, TINY16_EMU_SCREEN_HEIGHT}, //
                   (Vector2){0, 0}, 0.0f, WHITE);
    DrawFPS(10, 10);
    EndDrawing();
}

bool tiny16_emu_run_cli(Tiny16VM* vm, uint32_t max_instructions) {
    return tiny16_vm_exec(vm, max_instructions);
}
