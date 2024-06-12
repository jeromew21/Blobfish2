#include "chess.h"
#include "perft.h"
#include "search.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void board_make_move_from_alg(Board *board, const char *algebraic);

int main() {
  srand(0);
  Board *board = board_default_starting_position();
  char buffer[8192]; // what's max input len?
  char move_bufer[16];
  while (1) {
    dump_board(board);
    f64 eval = evaluation(board, kWhite);
    printf("Eval: %f centipawns\n", eval);
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
      buffer[strlen(buffer) - 1] = '\0';
      board_make_move_from_alg(board, buffer);
      bool stop = false;
      Move best_move;
      search(board, &stop, &best_move);
        move_to_string(best_move, move_bufer);
        printf("Found move %s\n", move_bufer);
      board_make_move(board, best_move);
    } else {
      break;
    }
  }
  free(board);
}

/**
 * Given an algebraic move like "a2b1q", make the move on board.
 * This shouldn't be called within a loop.
 * TODO: promotions.
 */
void board_make_move_from_alg(Board *board, const char *algebraic) {
  static char row_names[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
  static char col_names[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
  u32 r0, c0, r1, c1;
  for (u32 i = 0; i < 8; i++) {
    if (algebraic[0] == col_names[i])
      c0 = i;
    if (algebraic[1] == row_names[i])
      r0 = i;
    if (algebraic[2] == col_names[i])
      c1 = i;
    if (algebraic[3] == row_names[i])
      r1 = i;
  }
  u32 idx_src = r0 * 8 + c0;
  u32 idx_dest = r1 * 8 + c1;
  MoveList moves = generate_all_legal_moves(board);
  Move mv;
  bool found = false;
  for (int i = 0; i < moves.count; i++) {
    Move candidate = move_list_get(&moves, i);
    u32 candidate_src = move_get_src_u32(candidate);
    u32 candidate_dest = move_get_dest_u32(candidate);
    if (idx_src == candidate_src && idx_dest == candidate_dest) {
      mv = candidate;
      found = true;
      break;
    }
  }
  if (!found) {
    // report some error
    return;
  }
  board_make_move(board, mv);
}

// TODO: fix this
void move_to_string(Move mv, char *buf) {
  // returns string of length 5, i.e. e2-e4
  static char row_names[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
  static char col_names[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
  u32 src = move_get_src_u32(mv);
  u32 dest = move_get_dest_u32(mv);
  // printf("%c%c-%c%c", col_names[src%8], row_names[src/8], col_names[dest%8],
  // row_names[dest/8]);
  char special[256];
  special[0] = '\0';
  u32 md = move_get_metadata(mv);
  if (md == kEnPassantMove) {
    sprintf(special, "%s", " en passant");
  } else if (md == kKingSideCastleMove) {
    sprintf(special, "%s", " O-O");
  } else if (md == kQueenSideCastleMove) {
    sprintf(special, "%s", " O-O-O");
  } else if (md & 0b1000) {
    if (md == kQueenPromotionMove) {
      sprintf(special, "%s", " Q");
    } else if (md == kQueenCapturePromotionMove) {
      sprintf(special, "%s", " Q Capture");
    } else {
      sprintf(special, "%s", " under promotion");
    }
  }
  sprintf(buf, "%c%c%c%c%s", col_names[src % 8], row_names[src / 8],
          col_names[dest % 8], row_names[dest / 8], special);
}
