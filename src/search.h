#pragma once

#include "chess.h"
#include "uci.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

typedef int32_t Centipawns;

static const Centipawns MIN_EVAL = -100000;

/* Evaluation */

Centipawns evaluation(Board *board, i32 side);

/* Search */

void search_uci(EngineContext *ctx);

Centipawns qsearch(Board *board, Centipawns alpha, Centipawns beta,
                   _Atomic(bool) *stop);

Centipawns search_recursive(Board *board, Centipawns alpha, Centipawns beta,
                            i32 depth, _Atomic(bool) *stop, Move *best_move);
