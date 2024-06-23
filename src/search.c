#include "search.h"
#include "chess.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TranspositionTable tt;

void init_tables();

void destroy_tables();

TTableBucket *ttable_probe(u64 hash); // TODO

PVTableBucket *pv_table_probe();

typedef struct SearchArguments {
  Board *board;
  Centipawns alpha;
  Centipawns beta;
  i32 ply_depth;
  AtomicBool *stop;
  u64 *nodes_searched;
} SearchArguments;

typedef struct ScoredMove {
  Move mv;
  i32 score;
  bool valid;
} ScoredMove;

typedef struct ScoredMoveList {
  i32 count;
  ScoredMove items[256];
} ScoredMoveList;

Centipawns search_recursive(SearchArguments args);

Centipawns qsearch(Board *board, Centipawns alpha, Centipawns beta,
                   AtomicBool *stop);

Move pop_max(ScoredMoveList *scored_moves);

Move peep_max(ScoredMoveList *scored_moves);

Centipawns min(Centipawns x, Centipawns y) { return x < y ? x : y; }

Centipawns max(Centipawns x, Centipawns y) { return x > y ? x : y; }

/**
 * Root search
 */
void search(Board *board, Move *best_move, AtomicBool *stop_thinking,
            FILE *outfile) {
  // TODO: multi threading
  // TODO: allow search subset of moves (this is a UCI requirement)
  // TODO: don't return best move in recursive impl, use root node search
  // TODO: do we alpha-beta prune at root? why or why not?
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  int ply_depth = 0;
  u64 nodes_searched = 0;
  MoveList moves = generate_all_legal_moves(board);
  (*best_move) = move_list_get(&moves, 0);
  while (1) {
    if (*stop_thinking)
      return;
    Move best_move_found = move_list_get(&moves, 0);
    Centipawns best_score_found = MIN_EVAL;
    Centipawns alpha = MIN_EVAL;
    Centipawns beta = -MIN_EVAL;
    for (int i = 0; i < moves.count; i++) {
      if (*stop_thinking)
        return; // we want to return best move from previous depth
      // maybe we want to keep this if we beat the previous depth best
      // score, not sure
      Move mv = move_list_get(&moves, i);
      board_make_move(board, mv);
      SearchArguments sub_args;
      sub_args.board = board;
      sub_args.alpha = -beta;
      sub_args.beta = -alpha;
      sub_args.ply_depth = ply_depth;
      sub_args.stop = stop_thinking;
      sub_args.nodes_searched = &nodes_searched;
      Centipawns score = -search_recursive(sub_args);
      if (score > best_score_found) {
        best_score_found = score;
        best_move_found = mv;
      }
      board_unmake(board);
    }
    if (*stop_thinking)
      return;
    (*best_move) = best_move_found;
    if (outfile) {
      // check for mate
      char score_string[64];
      bool mate = false;
      if (abs(MIN_EVAL) - abs(best_score_found) < 1024) {
        // 1024 leaves room for, say, mate in 30, with a large game of around
        // 400+ plies. mate in n now plies = distance from board 0th ply to
        // leaf.depth
        if (best_score_found > 0) {
          int plies = -MIN_EVAL - best_score_found;
          int moves_to_mate = ceil(((double)(plies - board->_ply)) / 2.0);
          sprintf(score_string, "mate %i", moves_to_mate);
        } else {
          int plies = best_score_found - MIN_EVAL;
          int moves_to_mate = ceil(((double)(plies - board->_ply)) / 2.0);
          sprintf(score_string, "mate -%i", moves_to_mate);
        }
        mate = true;
      } else {
        sprintf(score_string, "cp %i", best_score_found);
      }
      struct timespec tick;
      clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
      // Somehow, maybe bc of floating point error, this outputs badly for small
      // values.
      // TODO: fix precision
      u64 execution_time_ms = (tick.tv_sec - start.tv_sec) * 1000 +
                              (tick.tv_nsec - start.tv_nsec) / 1000000;
      if (execution_time_ms == 0) {
        execution_time_ms = 1;
      }
      double npms = (double)nodes_searched / (double)execution_time_ms;
      double nps = npms * 1000;
      double hashfull = 1000.0 * (double)tt.filled / (double)tt.count;
      char pv[8192];
      pv[0] = '\0';
      {
        Move mv = best_move_found;
        char buf[16];
        int i = 0;
        while (i < ply_depth + 1) {
          MoveList legals = generate_all_legal_moves(board);
          bool mv_is_legal = false;
          for (int k = 0; k < legals.count; k++) {
            if (mv == move_list_get(&legals, k)) {
              mv_is_legal = true;
              break;
            }
          }
          if (!mv_is_legal) {
            break;
          }
          move_to_string(mv, buf);
          board_make_move(board, mv);
          sprintf(pv + strlen(pv), " %s", buf);
          i++;
          TTableBucket *bucket =
              ttable_probe(board_metadata_peek(board, 0)->_hash);
          if (bucket->hash == 0) {
            break;
          }
          if (bucket->node_type != kPV) {
            break;
          }
          mv = bucket->best_move;
        }
        for (int k = 0; k < i; k++) {
          board_unmake(board);
        }
      }
      fprintf(
          outfile,
          "info depth %i score %s nodes %llu nps %i hashfull %i time %i pv%s\n",
          ply_depth + 1, score_string, (unsigned long long)nodes_searched,
          (int)nps, (int)hashfull, (int)execution_time_ms, pv);
      if (mate) {
        // Stockfish does keep outputting higher depths when mate found but not
        // sure if this is due to parallel search or something
        return;
      }
    }
    ply_depth++;
  }
}

/**
 * Quiescience search
 */
Centipawns qsearch(Board *board, Centipawns alpha, Centipawns beta,
                   AtomicBool *stop) {
  int stand_pat = evaluation(board);
  if (stand_pat >= beta) {
    return beta;
  }
  if (alpha < stand_pat) {
    alpha = stand_pat;
  }
  MoveList capture_moves = generate_capture_moves(board);
  for (int i = 0; i < capture_moves.count; i++) {
    if (*stop) {
      return alpha;
    }
    Move mv = move_list_get(&capture_moves, i);
    board_make_move(board, mv);
    Centipawns score = -qsearch(board, -beta, -alpha, stop);
    board_unmake(board);
    if (score >= beta) {
      return beta;
    }
    if (score > alpha) {
      alpha = score;
    }
  }
  return alpha;
}

/**
 * Our workhorse Alpha-Beta Search
 * TODO: PVS
 */
Centipawns search_recursive(SearchArguments args) {
  (*args.nodes_searched)++;
  Move tt_move = 0;
  u64 hash = board_metadata_peek(args.board, 0)->_hash;
  TTableBucket *bucket_ptr = ttable_probe(hash);
  TTableBucket bucket_prev = *bucket_ptr;
  TTableBucket bucket = *bucket_ptr;
  if (bucket.hash == hash) {
    tt_move = bucket.best_move;
    if (bucket.depth >= args.ply_depth) {
      switch (bucket.node_type) {
      case kCut:
        args.alpha = max(args.alpha, bucket.score);
        break;
      case kAll:
        args.beta = min(args.beta, bucket.score);
        break;
      case kPV: {
        return bucket.score;
      }
      }
      if (args.alpha >= args.beta) {
        return args.beta;
      }
      // do we need this?
      return bucket.score;
    }
  }
  i32 status = board_status(args.board);
  if (status == kCheckmate) {
    Centipawns mating_score = MIN_EVAL + (i32)args.board->_ply;
    return mating_score;
  }
  if (status == kStalemate || status == kDraw) {
    return 0; // TODO: contempt factor
  }
  if (args.ply_depth == 0) {
    Centipawns qscore = qsearch(args.board, args.alpha, args.beta, args.stop);
    return qscore;
  }
  // WHY DO WE START INSERTING AFTER HERE???
  // surely we want to store leaf results??
  if (bucket.hash == 0) {
    tt.filled += 1;
  }
  bucket.hash = hash;
  bucket.depth = args.ply_depth;
  MoveList legal_moves = generate_all_legal_moves(args.board);
  ScoredMoveList moves_scored;
  moves_scored.count = 0;
  for (i32 i = 0; i < legal_moves.count; i++) {
    Move mv = move_list_get(&legal_moves, i);
    u32 mv_md = move_get_metadata(mv);
    u64 src = move_get_src(mv);
    i32 score = 0;
    if (mv == tt_move) {
      score += 1000;
    }
    if (mv_md & PROMOTION_BIT_FLAG) {
      score += 100;
    }
    if (mv_md & CAPTURE_BIT_FLAG) {
      score += 100;
    }
    if (src & args.board->_bitboard[kPawn]) {
      score += 10;
    }
    moves_scored.items[moves_scored.count].mv = mv;
    moves_scored.items[moves_scored.count].score = score;
    moves_scored.items[moves_scored.count].valid = true;
    moves_scored.count++;
  }
  bucket.best_move = peep_max(&moves_scored);
  bucket.node_type = kAll; // Default is all-node, an upper bound, where exact
                           // score might be lower No move raises alpha
  for (int i = 0; i < legal_moves.count; i++) {
    if (*args.stop) {
      return args.alpha;
    }
    Move mv = pop_max(&moves_scored);
    board_make_move(args.board, mv);
    SearchArguments sub_args;
    sub_args.board = args.board;
    sub_args.alpha = -args.beta;
    sub_args.beta = -args.alpha;
    sub_args.ply_depth = args.ply_depth - 1;
    sub_args.stop = args.stop;
    sub_args.nodes_searched = args.nodes_searched;
    Centipawns score = -search_recursive(sub_args);
    board_unmake(args.board);
    if (score >= args.beta) {
      // this is a Cut-node
      // we return a lower bound; the exact score might be higher
      bucket.node_type = kCut;
      bucket.best_move = mv;
      args.alpha = args.beta;
      // bucket.score = args.beta;
      break;
      //(*bucket_ptr) = bucket;
      // return args.beta;
    }
    if (score > args.alpha) {
      bucket.node_type = kPV;
      bucket.best_move = mv;
      args.alpha = score;
    }
  }
  bucket.score = args.alpha;
  bool eviction_cond = bucket_prev.node_type != kPV || bucket_prev.hash == 0;
  if (eviction_cond) {
    (*bucket_ptr) = bucket;
  }
  return args.alpha;
}

Move pop_max(ScoredMoveList *scored_moves) {
  ScoredMove best;
  best.score = -100000000;
  best.mv = 0;
  int best_idx = 0;
  for (i32 i = 0; i < scored_moves->count; i++) {
    ScoredMove sm = scored_moves->items[i];
    if (!sm.valid) {
      continue;
    }
    if (sm.score > best.score) {
      best = sm;
      best_idx = i;
    }
  }
  scored_moves->items[best_idx].valid = false;
  return best.mv;
}

Move peep_max(ScoredMoveList *scored_moves) {
  ScoredMove best;
  best.score = -100000000;
  best.mv = 0;
  for (i32 i = 0; i < scored_moves->count; i++) {
    ScoredMove sm = scored_moves->items[i];
    if (!sm.valid) {
      continue;
    }
    if (sm.score > best.score) {
      best = sm;
    }
  }
  return best.mv;
}

TTableBucket *ttable_probe(u64 hash) {
  TTableBucket *bucket = &tt.buckets[hash & tt.mask];
  return bucket;
}

// TODO: add parameters
void init_tables() {
  {
    const u64 target_table_size_mib = 16; // 1mib = 1024 x 1024 bytes
    const u64 target_table_size_b = target_table_size_mib * 1024 * 1024;
    const u64 bucket_size_b = sizeof(TTableBucket);
    int n = 0;
    u64 count;
    while (1) {
      count = (u64)1 << n;
      const u64 table_size_b = count * bucket_size_b;
      if (table_size_b > target_table_size_b) {
        count = (u64)1 << (n - 1);
        break;
      }
      n++;
    }
    tt.count = count;
    tt.filled = 0;
    tt.buckets = calloc(1, count * bucket_size_b);
    tt.mask = count - 1;
    printf("info transposition table size %i mib\n",
           (int)(count * bucket_size_b / (1024 * 1024)));
    printf("info transposition table count %llu elements\n",
           (long long unsigned)tt.count);
  }
  {
    // TODO allocate for pv table
  }
}

void destroy_tables() { free(tt.buckets); }
