#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

#include "search.h"
#include "chess.h"
#include <float.h>

f64 search_recursive(Board *board, f64 alpha, f64 beta, i32 depth, bool *stop,
                     Move *best_move);

void search(Board *board, bool *stop, Move *best_move) {
  // TODO: iterative deepening
  // when you stop, cancel the entire depth level, and return best move of
  // previous
  const int depth = 3;
  search_recursive(board, MIN_EVAL, MAX_EVAL, depth, stop, best_move);
}

f64 search_recursive(Board *board, f64 alpha, f64 beta, i32 depth, bool *stop,
                     Move *best_move) {
  if (depth == 0) {
    return evaluation(board, board->_turn);
  }
  MoveList moves = generate_all_legal_moves(board);
  Move best_move_found;
  for (int i = 0; i < moves.count; i++) {
    Move mv = move_list_get(&moves, i);
    board_make_move(board, mv);
    f64 score =
        -search_recursive(board, -beta, -alpha, depth - 1, stop, best_move);
    board_unmake(board);
    if (score >= beta) {
      (*best_move) = best_move_found; // do we need? we're not in a PV-node
      // this is a Cut-node
      // we return a lower bound; the exact score might be higher
      return beta;
    }
    if (score > alpha) {
      alpha = score;
      best_move_found = mv;
    }
    if (*stop) {
      // now we don't care about the result here
      break;
    }
  }
  (*best_move) = best_move_found;
  return alpha;
}

#pragma clang diagnostic pop
