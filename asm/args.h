#pragma once

#define REQUIRED_ARGS                                                                              \
    REQUIRED_STRING_ARG(source_filename, "source", "Path to the source tiny16 ASM file")           \
    REQUIRED_STRING_ARG(output_filename, "output", "Path to the output tiny16 BIN file")

#define BOOLEAN_ARGS BOOLEAN_ARG(help, "-h", "Show help")

#include "easyargs.h"

extern args_t args;

void make_and_parse_args(int argc, char** argv);
