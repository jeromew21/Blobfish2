#pragma once

#include "chess.h"
#include "uci.h"
#include <stdbool.h>
#include <stdint.h>

typedef int32_t Centipawns;

static const Centipawns MIN_EVAL = -1000000;

enum NodeType {
    kPV = 1,
    kCut = 2,
    kAll = 3,
};

typedef struct TTableBucket {
    u64 hash; // 0 if not present
    u8 depth;
    u8 node_type;
    Move best_move;
    Centipawns score;
} TTableBucket;

typedef struct TranspositionTable {
    TTableBucket *buckets;
    u64 count;
    u64 mask;
    u64 filled;
} TranspositionTable;

/*
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
*/

typedef struct KillerTableBucket {
    Move mv;
    u32 root_distance;
} KillerTableBucket;

typedef struct KillerTable {
    KillerTableBucket *buckets;
    u64 count;
    u64 mask;
} KillerTable;

/* Evaluation */

Centipawns evaluation(Board *board);

/* Search */

void search(Board *board, Move *best_move, AtomicBool *stop_thinking,
            FILE *outfile, int depth_limit);

void init_tables(void);

void destroy_tables(void);
