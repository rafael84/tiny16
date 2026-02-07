#pragma once

#define REQUIRED_ARGS                                                                              \
    REQUIRED_STRING_ARG(source_filename, "source", "Path to the source tiny16se file (.se)")       \
    REQUIRED_STRING_ARG(output_filename, "output", "Path to the output tiny16 ASM file (.asm)")

#define BOOLEAN_ARGS BOOLEAN_ARG(help, "-h", "Show help")

#define OPTIONAL_ARGS                                                                              \
    OPTIONAL_STRING_ARG(search_path, "stdlib/se", "-I", "path",                                    \
                        "Search path for required modules")                                        \
    OPTIONAL_STRING_ARG(opt_level, "0", "-O", "level", "Optimization level (0, 1, or 2)")

#include "easyargs.h"

extern args_t args;

void make_and_parse_args(int argc, char** argv);
