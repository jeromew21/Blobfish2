#pragma once

#include "chess.h"
#include "pthread.h"

typedef struct EngineContext {
  // options, etc
  bool debug;
  bool quit;
  Move best_move; // maybe hold thread, idk
  bool stop_thinking;

  // conditional compilation?
  pthread_t think_thread;
} EngineContext;

void engine_initialize();

void engine_cleanup();

EngineContext *engine_get_context();

void engine_command(char *line_buffer);
