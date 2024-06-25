#pragma once

#include "chess.h"
#include <stdio.h>

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <stdatomic.h>
#include <pthread.h>
#define THREAD pthread_t
#define THREAD_CREATE pthread_create
#define THREAD_JOIN pthread_join
#define THREAD_DETACH pthread_detach
typedef _Atomic(bool) AtomicBool;
#elif defined(_WIN32) || defined(WIN32)
#include <windows.h>
typedef bool AtomicBool; // TODO: get atomics on Windows
#define THREAD HANDLE
void THREAD_CREATE(THREAD* t, void* attr, LPTHREAD_START_ROUTINE f, void*arg);
void THREAD_DETACH(THREAD t);
#endif

typedef struct EngineContext {
  FILE *log_fp;
  bool debug;
  bool quit;
  Move best_move;
  f64 think_time_ms;
  AtomicBool stop_thinking;
  Board *board;
} EngineContext;

void engine_initialize(void);

void engine_cleanup(void);

EngineContext *engine_get_context(void);

void engine_command(char *line_buffer);

bool board_make_move_from_alg(Board *board, const char *algebraic);
