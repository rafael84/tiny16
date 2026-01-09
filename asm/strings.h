#pragma once

#include <stdbool.h>

#include "context.h"

void tiny16_asm_str_strip_comment(char* str);
void tiny16_asm_str_trim_left(char* str);
void tiny16_asm_str_trim_right(char* str);
void tiny16_asm_str_trim(char* str);
long tiny16_asm_str_to_long(const char* str);
bool tiny16_asm_str_has_prefix(const char* pre, const char* str);

bool tiny16_asm_is_valid_label_prefix(char* str);
int tiny16_asm_label_length(char* str);
tiny16_asm_section_t tiny16_asm_section(char* str);
