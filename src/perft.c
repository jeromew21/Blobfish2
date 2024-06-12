#include "perft.h"
#include "chess.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

PerftResults perft_helper(Board *board, int depth, int top_depth);

void perft_performance_test() {
  struct timespec start, stop;
  Board *board = board_uninitialized();
  board_initialize_startpos(board);
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  PerftResults pr = perft(board, 5);
  clock_gettime(CLOCK_MONOTONIC_RAW, &stop);
  u64 execution_time_ms = (stop.tv_sec - start.tv_sec) * 1000 +
                          (stop.tv_nsec - start.tv_nsec) / 1000000;
  printf("Perft 5 = %lu\n", pr.nodes);
  printf("Correct = 4865609\n");
  printf("Time: %lu ms\n", execution_time_ms);
  free(board);
}

bool parse_depth_count(char *line, int *i, int *out_depth, int *out_count) {
  int depth_start;
  int count_start = -1;
  if (*i >= (int)strlen(line))
    return false;
  char c = '\0';
  while (c != ';') {
    c = line[*i];
    if (c == '\0')
      break;
    if (c == 'D') {
      depth_start = (*i) + 1;
    } else if (c == ' ' && count_start == -1) {
      count_start = (*i) + 1;
    }
    (*i)++;
  }
  *out_depth = atoi(line + depth_start);
  *out_count = atoi(line + count_start);
  return true;
}

bool perft_test_line(char *line, int max_depth) {
  int i = 0;
  char c = '\0';
  while (c != ';') {
    c = line[i];
    i++;
  }
  int depth, count;
  Board *board = board_uninitialized();
  board_initialize_fen(board, line);
  int cases = 0;
  int passes = 0;
  while (1) {
    bool parse_result = parse_depth_count(line, &i, &depth, &count);
    if (!parse_result) {
      break;
    }
    if (depth > max_depth)
      return true;
    PerftResults pr = perft(board, depth);
    cases++;
    if ((int)pr.nodes == count) {
      passes++;
    }
  }
  free(board);
  if (passes == cases) {
    printf("PASSED %i cases\n", cases);
    return true;
  } else {
    printf("FAIL\n");
    return false;
  }
}

void perft_test_from_file(const char *filename, int max_depth) {
  FILE *fp;
#define LINE_BUFFER_SIZE 1024
  char buffer[LINE_BUFFER_SIZE];
  fp = fopen(filename, "r");
  int cases = 0;
  int passes = 0;
  while (fgets(buffer, LINE_BUFFER_SIZE, fp)) {
    if (perft_test_line(buffer, max_depth)) {
      passes++;
    }
    cases++;
  }
#undef LINE_BUFFER_SIZE
  fclose(fp);
  if (passes == cases) {
    printf("Passed all test cases.");
  }
}

PerftResults perft(Board *board, int depth) {
  return perft_helper(board, depth, depth);
}

PerftResults perft_helper(Board *board, int depth, int top_depth) {
  PerftResults r;
  if (depth == 0) {
    r.nodes = 1;
    r.castles = 0;
    r.ep = 0;
    r.promotions = 0;
    r.captures = 0;
    return r;
  }
  r.nodes = 0;
  r.castles = 0;
  r.ep = 0;
  r.promotions = 0;
  r.captures = 0;
  MoveList moves = generate_all_legal_moves(board);
  for (int i = 0; i < moves.count; i++) {
    Move mv = move_list_get(&moves, i);
    u32 move_md = move_get_metadata(mv);
    if (move_md & kCaptureMove) {
      r.captures++;
    }
    if (move_md == kEnPassantMove) {
      r.ep++;
    }
    if (move_md == kQueenSideCastleMove || move_md == kKingSideCastleMove) {
      r.castles++;
    }
    if (move_md & PROMOTION_BIT_FLAG) {
      r.promotions++;
    }
    board_make_move(board, mv);
    PerftResults sub_results = perft_helper(board, depth - 1, top_depth);
    r.nodes += sub_results.nodes;
    r.captures += sub_results.captures;
    r.ep += sub_results.ep;
    r.castles += sub_results.castles;
    r.promotions += sub_results.promotions;
    board_unmake(board);
  }
  return r;
}
