#pragma once

#include "chess.h"

typedef struct PerftResults {
  u64 nodes;
  u64 captures;
  u64 ep;
  u64 castles;
  u64 promotions;
} PerftResults;

PerftResults perft(Board *board, int depth);

void perft_performance_test(void);

void perft_test_from_file(const char *filename, int maxdepth);

void puzzle_test(const char *puzzle_db_csv);

void hashing_test();
