#pragma once

#include "chess.h"
#include <stdatomic.h>
#include <stdio.h>

/*
 * Threading abstraction. We might have to use a function macro or define
 * functions for create/join based on what the Windows API looks like.
 */
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include "pthread.h"
#define THREAD pthread_t
#define THREAD_CREATE pthread_create
#define THREAD_JOIN pthread_join
#elif defined(_WIN32) || defined(WIN32)
#include "windows.h"
#define THREAD windows_thread_TODO
#endif

typedef struct EngineContext {
  FILE *log_fp;
  bool debug;
  bool quit;
  Move best_move;
  _Atomic(bool) stop_thinking;
  Board *board;
  THREAD think_thread;
} EngineContext;

void engine_initialize(void);

void engine_cleanup(void);

EngineContext *engine_get_context(void);

void engine_command(char *line_buffer);
