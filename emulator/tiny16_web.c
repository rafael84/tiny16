#include <emscripten/emscripten.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.c"
#include "cpu.h"
#include "memory.c"
#include "memory.h"
#include "ppu.c"
#include "ppu.h"
#include "vm.c"
#include "vm.h"

#include "tiny16_core.h"

// Global state for web build
static Tiny16Emulator* g_emulator = NULL;

// Function callable from JavaScript to reload program
EMSCRIPTEN_KEEPALIVE
void tiny16_web_reload_program(void) {
    if (!g_emulator) return;

    // Reload program from virtual filesystem
    const char* program_path = "/program.tiny16";
    if (tiny16_memory_load_from_file(&g_emulator->vm->memory, program_path)) {
        // Reset CPU to start fresh
        tiny16_cpu_reset(&g_emulator->vm->cpu);
        g_emulator->frame_counter = 0;
        g_emulator->instr_acc = 0.0f;
        g_emulator->program_loaded = true;
        printf("Program reloaded successfully\n");
    } else {
        printf("Failed to reload program\n");
    }
}

// Emscripten main loop callback
static void update_frame_callback(void) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        emscripten_cancel_main_loop();
        return;
    }
    tiny16_emu_update_frame(g_emulator);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Tiny16VM* vm = tiny16_vm_create();

    bool program_loaded = false;

    // Try to load program from virtual filesystem
    const char* program_path = "/program.tiny16";
    if (tiny16_memory_load_from_file(&vm->memory, program_path)) {
        printf("Loaded program from %s\n", program_path);
        program_loaded = true;
    } else {
        printf("No program loaded yet. Use file input to load a .tiny16 file.\n");
    }

    tiny16_cpu_tracing = false;

    // Initialize window
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(TINY16_EMU_SCREEN_WIDTH, TINY16_EMU_SCREEN_HEIGHT, "tiny16 emulator");
    SetTargetFPS(60);

    // Create emulator
    g_emulator = tiny16_emu_create(vm, program_loaded);

    // Use Emscripten's main loop
    emscripten_set_main_loop(update_frame_callback, 0, 1);

    return EXIT_SUCCESS;
}
