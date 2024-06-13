#pragma once

#include "chess.h"
#include "uci.h"
#include <stdatomic.h>
#include <stdbool.h>

static const f64 MIN_EVAL = -100000;
static const f64 MAX_EVAL = 100000;

/* Evaluation */

f64 evaluation(Board *board, i32 side);

/* Search */

void search_uci(EngineContext *ctx);

f64 search_recursive(Board *board, f64 alpha, f64 beta, i32 depth,
                     _Atomic(bool) *stop, Move *best_move);
