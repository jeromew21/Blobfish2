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

// TODO: input file...
void search(Board* board, Move* best_move, _Atomic(bool) *stop_thinking, FILE* outfile);
