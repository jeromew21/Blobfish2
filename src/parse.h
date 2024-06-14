#pragma once

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

//bool is_alphanum(const char c);

bool is_whitespace(const char c);

bool strings_equal(char *a, char *b);

void eat_whitespace(char *line_buffer, int *i);

bool eat_word(char *line_buffer, char *word_buffer, int *i);
