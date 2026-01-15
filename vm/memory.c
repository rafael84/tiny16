#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binary.h"
#include "cpu.h"
#include "memory.h"

void tiny16_memory_print_byte(const Tiny16Memory* memory, int addr) {
    uint8_t byte = memory->bytes[addr];
    if (byte == 0) return;
    printf("        0x%04X | 0b" TINY16_BIN8_FMT " | 0x%02X | %d\n", addr, TINY16_BIN8(byte), byte,
           byte);
}

void tiny16_memory_print_segment(const Tiny16Memory* memory, int begin, int end, char dir) {
    if (dir == 'A') {
        for (int i = begin; i <= end; ++i) {
            tiny16_memory_print_byte(memory, i);
        }
    } else if (dir == 'D') {
        for (int i = end; i >= begin; --i) {
            tiny16_memory_print_byte(memory, i);
        }
    } else {
        assert(0 && "Invalid direction");
    }
}

void tiny16_memory_print(const Tiny16Memory* memory, bool framebuffer) {
    printf("\n\nMemory\n");

    printf("\n  Code Segment\n");
    for (int pc = TINY16_MEMORY_CODE_BEGIN; pc <= TINY16_MEMORY_CODE_END; pc += 3) {
        Tiny16OpCode opcode = memory->bytes[pc];
        if (opcode == TINY16_OPCODE_UNKNOWN) continue;
        uint8_t arg1 = memory->bytes[pc + 1];
        uint8_t arg2 = memory->bytes[pc + 2];
        tiny16_cpu_trace(pc, opcode, arg1, arg2);
        printf("\n");
    }

    printf("\n  Data Segment\n");
    tiny16_memory_print_segment(memory, TINY16_MEMORY_DATA_BEGIN, TINY16_MEMORY_DATA_END, 'A');

    printf("\n  Stack Segment\n");
    tiny16_memory_print_segment(memory, TINY16_MEMORY_STACK_BEGIN, TINY16_MEMORY_STACK_END, 'D');

    if (framebuffer) {
        printf("\n  Framebuffer\n");

        const char pixel_map[] = " .,:;irsXA253hMHGS#9B&@";
        int max_idx = sizeof(pixel_map) - 2;
        for (int y = 0; y < TINY16_FRAMEBUFFER_SIZE_HEIGHT; ++y) {
            printf("        ");
            for (int x = 0; x < TINY16_FRAMEBUFFER_SIZE_WIDTH; ++x) {
                uint8_t pixel = memory->bytes[TINY16_FRAMEBUFFER_ADDR(x, y)];
                uint8_t r = ((pixel >> 5) & 0x07) * 255 / 7;
                uint8_t g = ((pixel >> 2) & 0x07) * 255 / 7;
                uint8_t b = (pixel & 0x03) * 255 / 3;
                float lum = ((0.2126f * r) + (0.7152f * g) + (0.0722f * b)) / 255.0f;
                int idx = (int)(lum * max_idx + 0.5f);
                if (idx < 0) idx = 0;
                if (idx > max_idx) idx = max_idx;
                putchar(pixel_map[idx]);
                putchar(pixel_map[idx]);
            }
            putchar('\n');
        }
    }
}

void tiny16_memory_reset(Tiny16Memory* memory) { memset(memory->bytes, 0, TINY16_MEMORY_SIZE); }

size_t tiny16_memory_load_from_file(Tiny16Memory* memory, char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("could not open file");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size_t filelen = ftell(file);
    rewind(file);

    if (filelen > TINY16_MEMORY_STACK_END) {
        fclose(file);
        fprintf(stderr, "program too large\n");
        exit(1);
    }

    size_t n = fread(memory->bytes, 1, filelen, file);
    fclose(file);

    return n;
}
