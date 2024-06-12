#pragma once

#include "chess.h"
#include "stdbool.h"

static const f64 MIN_EVAL = -100000;
static const f64 MAX_EVAL = 100000;

/* Evaluation */

f64 evaluation(Board *board, i32 side);

/* Search */

void search(Board *board, bool *stop, Move *best_move);
