#pragma once

#include "chess.h"
#include "uci.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

typedef int32_t Centipawns;

static const Centipawns MIN_EVAL = -100000;

enum NodeType {
  kPV,
  kCut,
  kAll,
};

typedef struct TTableBucket {
  u64 hash;
  u8 depth;
  u8 node_type;
  Move best_move;
} TTableBucket;

typedef struct TranspositionTable {
  TTableBucket *buckets;
  u64 count;
  u64 mask;
} TranspositionTable;

typedef struct PVTableBucket {
  u64 hash;
  i32 depth;
  Move line[64];
} PVTableBucket;

typedef struct PVTable {
  PVTableBucket *buckets;
  u64 count;
  u64 mask;
} PVTable;

/* Evaluation */

Centipawns evaluation(Board *board, i32 side);

/* Search */

void search(Board *board, Move *best_move, AtomicBool *stop_thinking,
            FILE *outfile);

void init_tables();

void destroy_tables();
