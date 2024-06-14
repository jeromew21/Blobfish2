#include "parse.h"

//bool is_alphanum(const char c) { return isalpha(c) || isdigit(c); }

bool is_whitespace(const char c) { return c == ' ' || c == '\t'; }

bool strings_equal(char *a, char *b) { return strcmp(a, b) == 0; }

void eat_whitespace(char *line_buffer, int *i) {
  while (1) {
    const char c = line_buffer[*i];
    if (!is_whitespace(c)) {
      break;
    }
    (*i)++;
  }
}

/**
 * Move the pointer along, filling word_buffer with the first found word, null
 * terminated. Consume all following whitespace.
 * Returns false if there isn't another word to eat (i.e. null terminator.)
 */
bool eat_word(char *line_buffer, char *word_buffer, int *i) {
  int k = 0;
  while (1) {
    const char c = line_buffer[*i];
    if (is_whitespace(c)) {
      break;
    }
    if (c == '\n') {
      if (k == 0) // first character we're seeing is a newline
        return false;
      else
        break;
    }
    word_buffer[k++] = c;
    (*i)++;
  }
  word_buffer[k] = '\0';
  eat_whitespace(line_buffer, i);
  return true;
}
