#include "chess.h"
#include "bitboard_constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void board_make_move_from_alg(Board *board) {}

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

int main() {
  srand(0);
  struct timespec start, stop;
  Board *board = board_default_starting_position();
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  PerftResults pr = perft(board, 5);
  clock_gettime(CLOCK_MONOTONIC_RAW, &stop);
  u64 execution_time_ms = (stop.tv_sec - start.tv_sec) * 1000 +
                          (stop.tv_nsec - start.tv_nsec) / 1000000;
  printf("Perft 5 = %lu\n", pr.nodes);
  printf("Time: %lu ms\n", execution_time_ms);
  free(board);
}
