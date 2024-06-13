#include "chess.h"
#include "perft.h"
#include "uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_BUFFER_SIZE 32768

void read_input_into_buffer(char *line_buffer) {
  // Technically this line buffer could overflow...
  fgets(line_buffer, LINE_BUFFER_SIZE, stdin);
}

int main(void) {
  srand(0);
  setbuf(stdout, NULL);
  setbuf(stdin, NULL);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
  engine_initialize();
  char *line_buffer =
      malloc(sizeof(char) * LINE_BUFFER_SIZE); // what's max input len?
  while (1) {
    read_input_into_buffer(line_buffer);
    engine_command(line_buffer);
    EngineContext *ctx = engine_get_context();
    if (ctx->quit) {
      break;
    }
  }
  free(line_buffer);
  engine_cleanup();
}

#undef LINE_BUFFER_SIZE
