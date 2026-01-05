#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.c"
#include "cpu.h"
#include "memory.c"
#include "memory.h"

#define MAX_INSTRUCTIONS 300000

char* args_shift(int* argc, char*** argv);

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

    if (!tiny16_cpu_exec(&cpu, &memory, MAX_INSTRUCTIONS)) {
        tiny16_cpu_print(&cpu);
        tiny16_memory_print(&memory, false);
        return 1;
    }

    tiny16_cpu_print(&cpu);
    tiny16_memory_print(&memory, true);
    return 0;
}

char* args_shift(int* argc, char*** argv) {
    assert(*argc > 0 && "argc <= 0");
    --(*argc);
    return *(*argv)++;
}
