#include "search.h"
#include "chess.h"
#include <float.h>

void search_uci(EngineContext *ctx) {
  // TODO: multi threading
  // TODO: search subset of moves
  // when you stop, cancel the entire depth level, and return best move of
  // previous
  // there seems to be a bug with forced moves?
  // TODO: don't return best move in recursive impl, use root node search
  // TODO: alpha-beta prune at root? why or why not?
  int ply_depth = 0;
  MoveList moves = generate_all_legal_moves(ctx->board);
  ctx->best_move = move_list_get(&moves, 0);
  Board *board = ctx->board;
  while (1) {
    printf("info depth %i\n", ply_depth + 1);
    Move best_move_found;
    Move pv_move;
    f64 best_score_found = MIN_EVAL;
    f64 alpha = MIN_EVAL;
    f64 beta = MAX_EVAL;
    for (int i = 0; i < moves.count; i++) {
      Move mv = move_list_get(&moves, i);
      board_make_move(board, mv);
      f64 score = -search_recursive(board, -beta, -alpha, ply_depth,
                                    &ctx->stop_thinking, &pv_move);
      if (score > best_score_found) {
        best_score_found = score;
        best_move_found = mv;
      }
      board_unmake(board);
      if (ctx->stop_thinking)
        return; // or at least, break out of the while loop
    }
    ctx->best_move = best_move_found;
    ply_depth++;
    if (ply_depth > 3) {
      break;
    }
  }
}

f64 search_recursive(Board *board, f64 alpha, f64 beta, i32 ply_depth,
                     _Atomic(bool) *stop, Move *pv_move) {
  if (ply_depth == 0) {
    // TODO: quiescience
    return evaluation(board, board->_turn);
  }
  MoveList moves = generate_all_legal_moves(board);
  Move best_move_found;
  for (int i = 0; i < moves.count; i++) {
    Move mv = move_list_get(&moves, i);
    board_make_move(board, mv);
    f64 score =
        -search_recursive(board, -beta, -alpha, ply_depth - 1, stop, pv_move);
    board_unmake(board);
    if (score >= beta) {
      (*pv_move) = best_move_found; // do we need? we're not in a PV-node
      // this is a Cut-node
      // we return a lower bound; the exact score might be higher
      return beta;
    }
    if (score > alpha) {
      alpha = score;
      best_move_found = mv;
    }
    if (*stop) {
      // we don't care about the result here
      break;
    }
  }
  (*pv_move) = best_move_found;
  return alpha;
}
