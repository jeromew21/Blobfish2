#include "search.h"
#include "chess.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct SearchArguments {
  Board *board;
  Centipawns alpha;
  Centipawns beta;
  i32 ply_depth;
  AtomicBool *stop;
  Move *pv_move;
  u64 *nodes_searched;
} SearchArguments;

Centipawns search_recursive(SearchArguments args);

Centipawns qsearch(Board *board, Centipawns alpha, Centipawns beta,
                   AtomicBool *stop, u64 *nodes_searched);

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

Centipawns qsearch(Board *board, Centipawns alpha, Centipawns beta,
                   AtomicBool *stop, u64 *nodes_searched) {
  (*nodes_searched)++;
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
    Centipawns score = -qsearch(board, -beta, -alpha, stop, nodes_searched);
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

Centipawns search_recursive(SearchArguments args) {
  // transposition table probe
  i32 status = board_status(args.board);
  if (status == kCheckmate) {
    return MIN_EVAL + args.board->_ply;
  }
  if (status == kStalemate || status == kDraw) {
    return 0;
  }
  if (args.ply_depth == 0) {
    return qsearch(args.board, args.alpha, args.beta, args.stop,
                   args.nodes_searched);
  }
  (*args.nodes_searched)++;
  MoveList moves = generate_all_legal_moves(args.board);
  Move best_move_found = move_list_get(&moves, 0);
  for (int i = 0; i < moves.count; i++) {
    if (*args.stop) {
      // we don't care about the result here
      break;
    }
    Move mv = move_list_get(&moves, i);
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
