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

int main(int argc, char** argv) {
    args_t args = make_default_args();
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

    int result = EXIT_SUCCESS;

    // CLI mode (no graphics)
    if (args.max_instructions > 0) {
        result = tiny16_emu_run_cli(vm, args.max_instructions) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    // GUI mode
    else {
        SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
        InitWindow(TINY16_EMU_SCREEN_WIDTH, TINY16_EMU_SCREEN_HEIGHT, "tiny16 emulator");
        SetTargetFPS(60);

        Tiny16Emulator* emu = tiny16_emu_create(vm, true);

        while (!WindowShouldClose()) {
            if (IsKeyPressed(KEY_ESCAPE)) break;
            tiny16_emu_update_frame(emu);
        }

        tiny16_emu_destroy(emu);
        CloseWindow();
    }

    if (args.dump || args.dump_framebuffer) {
        puts("\nDUMP ===========================================\n");
        tiny16_cpu_print(&vm->cpu);
        tiny16_memory_print(&vm->memory, args.dump_framebuffer);
    }

    return result;
}
