#pragma once

#include "chess.h"
#include "stdbool.h"

/* Evaluation */

f64 evaluation(Board *board, i32 side);

/* Search */

void search(Board* board, bool *stop, Move* best_move);