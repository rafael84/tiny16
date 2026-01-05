#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binary.h"
#include "memory.h"

void memory_print_byte(const Memory* memory, int addr) {
    uint8_t byte = memory->bytes[addr];
    if (byte == 0) {
        return;
    }
    printf("        0x%04X | 0b" BIN8_FMT " | 0x%02X | %d\n", addr, BIN8(byte), byte, byte);
}

void memory_print_segment(const Memory* memory, int begin, int end, char dir) {
    if (dir == 'A') {
        for (int i = begin; i <= end; ++i) {
            memory_print_byte(memory, i);
        }
    } else if (dir == 'D') {
        for (int i = end; i >= begin; --i) {
            memory_print_byte(memory, i);
        }
    } else {
        assert(0 && "Invalid direction");
    }
}

void memory_print(const Memory* memory, bool framebuffer) {
    printf("\n\nMemory\n");

    printf("\n  Code Segment\n");
    memory_print_segment(memory, MEMORY_CODE_BEGIN, MEMORY_CODE_END, 'A');

    printf("\n  Data Segment\n");
    memory_print_segment(memory, MEMORY_DATA_BEGIN, MEMORY_DATA_END, 'A');

    printf("\n  Stack Segment\n");
    memory_print_segment(memory, MEMORY_STACK_BEGIN, MEMORY_STACK_END, 'D');

    if (framebuffer) {
        printf("\n  Framebuffer\n");
        for (int y = 0; y < MMIO_FRAMEBUFFER_SIZE_HEIGHT; ++y) {
            for (int x = 0; x < MMIO_FRAMEBUFFER_SIZE_WIDTH; ++x) {
                uint8_t pixel = memory->bytes[MMIO_FRAMEBUFFER_ADDR(x, y)];
                if (x == 0) {
                    printf("        ");
                }
                printf("%02X", pixel);
            }
            printf("\n");
        }
    }
}

void memory_reset(Memory* memory) { memset(memory->bytes, 0, MEMORY_SIZE); }

size_t memory_load_from_file(Memory* memory, char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("could not open file");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size_t filelen = ftell(file);
    rewind(file);

    if (filelen > (MEMORY_CODE_END - MEMORY_CODE_BEGIN)) {
        fclose(file);
        fprintf(stderr, "program too large\n");
        exit(1);
    }

    size_t n = fread(memory->bytes, 1, filelen, file);
    fclose(file);

    return n;
}
