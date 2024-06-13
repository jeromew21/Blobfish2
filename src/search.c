#include "search.h"
#include "chess.h"
#include <float.h>

void search_uci(EngineContext *ctx) {
  // TODO: multi threading
  // TODO: search subset of moves
  // TODO: don't return best move in recursive impl, use root node search
  // TODO: alpha-beta prune at root? why or why not?
  int ply_depth = 0;
  MoveList moves = generate_all_legal_moves(ctx->board);
  ctx->best_move = move_list_get(&moves, 0);
  Board *board = ctx->board;
  while (1) {
    if (ctx->stop_thinking)
      return;
    Move best_move_found = move_list_get(&moves, 0);
    Move pv_move;
    Centipawns best_score_found = MIN_EVAL;
    Centipawns alpha = MIN_EVAL;
    Centipawns beta = -MIN_EVAL;
    for (int i = 0; i < moves.count; i++) {
      if (ctx->stop_thinking)
        return; // or at least, break out of the while loop
      Move mv = move_list_get(&moves, i);
      board_make_move(board, mv);
      Centipawns score = -search_recursive(board, -beta, -alpha, ply_depth,
                                           &ctx->stop_thinking, &pv_move);
      if (score > best_score_found) {
        best_score_found = score;
        best_move_found = mv;
      }
      board_unmake(board);
    }
    ctx->best_move = best_move_found;
    printf("info depth %i score cp %i\n", ply_depth + 1, best_score_found);
    ply_depth++;
  }
}

Centipawns qsearch(Board *board, Centipawns alpha, Centipawns beta,
                   _Atomic(bool) *stop) {
  int stand_pat = evaluation(board, board->_turn);
  /*
  if (stand_pat >= beta) {
    return beta;
    }
    */
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

Centipawns search_recursive(Board *board, Centipawns alpha, Centipawns beta,
                            i32 ply_depth, _Atomic(bool) *stop, Move *pv_move) {
  if (ply_depth == 0) {
    return qsearch(board, alpha, beta,
                   stop); // evaluation(board, board->_turn);
  }
  MoveList moves = generate_all_legal_moves(board);
  Move best_move_found;
  for (int i = 0; i < moves.count; i++) {
    if (*stop) {
      // we don't care about the result here
      break;
    }
    Move mv = move_list_get(&moves, i);
    board_make_move(board, mv);
    Centipawns score =
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
  }
  (*pv_move) = best_move_found;
  return alpha;
}
