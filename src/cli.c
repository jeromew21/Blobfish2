#include "chess.h"
#include "perft.h"
#include "uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_BUFFER_SIZE 8192

void read_input_into_buffer(char *line_buffer) {
  // Technically this line buffer could overflow...
  // fgets(line_buffer, LINE_BUFFER_SIZE, stdin);
  // fgets doesn't work with cutechess
  // UCI protocol specifies that we expect ending newlines from the GUI, so
  // don't strip them.
  char buf[1];
  int i = 0;
  while (read(0, buf, sizeof(buf)) > 0) {
    // read() here read from stdin charachter by character
    // the buf[0] contains the character got by read()
    line_buffer[i++] = buf[0];
    if (buf[0] == '\n')
      break;
  }
  line_buffer[i] = '\0';
}

int main() {
  srand(0);
  setbuf(stdout, NULL);
  setbuf(stdin, NULL);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
  // perft_performance_test();
  engine_initialize();
  FILE *fp = fopen("log.txt", "a");
  char line_buffer[LINE_BUFFER_SIZE]; // what's max input len?
  while (1) {
    read_input_into_buffer(line_buffer);
    fwrite(line_buffer, sizeof(char), strlen(line_buffer), fp);
    fflush(fp);
    engine_command(line_buffer);
    EngineContext *ctx = engine_get_context();
    if (ctx->quit) {
      break;
    }
  }
  engine_cleanup();
  fclose(fp);
}

#undef LINE_BUFFER_SIZE
