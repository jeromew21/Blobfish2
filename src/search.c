#include "search.h"
#include "chess.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

TranspositionTable tt;

void init_tables();

void destroy_tables();

TTableBucket *ttable_probe(); // TODO

void ttable_insert(); // TODO

PVTableBucket *pv_table_probe(); // TODO

void pv_table_insert(); // TODO

typedef struct SearchArguments {
  Board *board;
  Centipawns alpha;
  Centipawns beta;
  i32 ply_depth;
  AtomicBool *stop;
  Move *pv_move;
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
    Move pv_move;
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
      sub_args.pv_move = &pv_move;
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
    if (best_score_found)
      (*best_move) = best_move_found;
    if (outfile) {
      // check for mate
      char score_string[64];
      bool mate = false;
      if (abs(MIN_EVAL) - abs(best_score_found) < 100) {
        // mate in n
        // now plies = distance from board 0th ply to leaf.depth
        if (best_score_found > 0) {
          int plies = -MIN_EVAL - best_score_found;
          int moves_to_mate = ceil(((double)(plies - board->_ply)) / 2.0);
          sprintf(score_string, "mate %i", moves_to_mate);
        } else {
          printf("info best score found = %i\n", best_score_found);
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
      fprintf(outfile, "info depth %i score %s nodes %llu nps %i time %i\n",
              ply_depth + 1, score_string, (unsigned long long)nodes_searched,
              (int)nps, (int)execution_time_ms);
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
  int stand_pat = evaluation(board, board->_turn);
  if (stand_pat >= beta) {
    return beta;
  }
  if (alpha < stand_pat) {
    alpha = stand_pat;
  }
  MoveList capture_moves = generate_capture_moves(board);
  for (int i = 0; i < capture_moves.count; i++) {
    if (*stop) {
      break;
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
  // transposition table probe
  i32 status = board_status(args.board);
  if (status == kCheckmate) {
    return MIN_EVAL + (i32)args.board->_ply;
  }
  if (status == kStalemate || status == kDraw) {
    return 0;
  }
  if (args.ply_depth == 0) {
    return qsearch(args.board, args.alpha, args.beta, args.stop);
  }
  MoveList legal_moves = generate_all_legal_moves(args.board);
  ScoredMoveList moves_scored;
  moves_scored.count = 0;
  for (i32 i = 0; i < legal_moves.count; i++) {
    Move mv = move_list_get(&legal_moves, i);
    u32 mv_md = move_get_metadata(mv);
    i32 score = 0;
    if (mv_md & PROMOTION_BIT_FLAG) {
      score += 100;
    }
    if (mv_md & CAPTURE_BIT_FLAG) {
      score += 100;
    }
    moves_scored.items[moves_scored.count].mv = mv;
    moves_scored.items[moves_scored.count].score = score;
    moves_scored.items[moves_scored.count].valid = true;
    moves_scored.count++;
  }
  Move best_move_found = peep_max(&moves_scored);
  for (int i = 0; i < legal_moves.count; i++) {
    if (*args.stop) {
      // we don't care about the result here
      break;
    }
    Move mv = pop_max(&moves_scored); // move_list_get(&legal_moves, i);
    board_make_move(args.board, mv);
    SearchArguments sub_args;
    sub_args.board = args.board;
    sub_args.alpha = -args.beta;
    sub_args.beta = -args.alpha;
    sub_args.ply_depth = args.ply_depth - 1;
    sub_args.stop = args.stop;
    sub_args.pv_move = args.pv_move;
    sub_args.nodes_searched = args.nodes_searched;
    Centipawns score = -search_recursive(sub_args);
    board_unmake(args.board);
    if (score >= args.beta) {
      (*args.pv_move) = best_move_found; // do we need? we're not in a PV-node
      // this is a Cut-node
      // we return a lower bound; the exact score might be higher
      return args.beta;
    }
    if (score > args.alpha) {
      args.alpha = score;
      best_move_found = mv;
    }
  }
  // what if no move raised alpha? then do we store?
  (*args.pv_move) = best_move_found;
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

// TODO: add some parameters
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
    tt.buckets = malloc(count * bucket_size_b);
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
