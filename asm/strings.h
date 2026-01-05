#pragma once

#include <stdbool.h>

void tiny16_asm_str_strip_comment(char* str);
void tiny16_asm_str_trim_left(char* str);
void tiny16_asm_str_trim_right(char* str);
void tiny16_asm_str_trim(char* str);
void tiny16_asm_str_to_upper(char* str);
long tiny16_asm_str_to_long(const char* str);
bool tiny16_asm_is_valid_label_prefix(char* str);
int tiny16_asm_label_length(char* str);
