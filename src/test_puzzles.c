#include "chess.h"
#include "parse.h"
#include "search.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

static AtomicBool stop_thinking;
static Move best_move;
static u64 think_ms;

bool eat_line_until_delim(const char *buffer, char *cell_buffer, char delimiter,
                          int *i);

// TODO: if we end up wanting to multi-thread this,
// we have to define a struct for arguments, and allocate it on the heap.
void *puzzle_think_timer(void *_unused) {
  (void)_unused;
  printf("Thinking for %i ms\n", (int)think_ms);
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  while (1) {
    struct timespec tick;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
    u64 execution_time_ms = (tick.tv_sec - start.tv_sec) * 1000 +
                            (tick.tv_nsec - start.tv_nsec) / 1000000;
    if (execution_time_ms >= think_ms) {
      stop_thinking = true;
      break;
    }
    if (stop_thinking) {
      break;
    }
  }
  return NULL;
}

// TODO: return more comprehensive result (i.e. rating)
// TODO: any checkmate is good
bool puzzle_test_line(const char *line, Board *board) {
#define FIELD_SIZE 1024
#define FIELD_COUNT 10
  enum FieldNames {
    PuzzleId,
    FEN,
    Moves,
    Rating,
    RatingDeviation,
    Popularity,
    NbPlays,
    Themes,
    GameUrl,
    OpeningTags
  };
  int i = 0;
  char cell_buffer[FIELD_SIZE];
  char cells[FIELD_COUNT][FIELD_SIZE];
  int cell = 0;
  while (eat_line_until_delim(line, cell_buffer, ',', &i)) {
    memcpy(cells[cell], cell_buffer, sizeof(char) * FIELD_SIZE);
    cell++;
  }
  printf("=== PUZZLE %s ===\n", cells[PuzzleId]);
  /*
  printf("%s\n", cells[PuzzleId]);
  printf("%s\n", cells[FEN]);
  printf("%s\n", cells[Moves]);
   */
  board_initialize_fen(board, cells[FEN], NULL);
  char second_move_buf[16];
  {
    int move_parse_head = 0;
    char first_move_buf[16];
    eat_word(cells[Moves], first_move_buf, &move_parse_head);
    eat_word(cells[Moves], second_move_buf, &move_parse_head);
    board_make_move_from_alg(board, first_move_buf);
  }
  think_ms = 100;
  stop_thinking = false;
  //THREAD think_timer_thread;
  //THREAD_CREATE(&think_timer_thread, NULL, puzzle_think_timer, (void *)NULL);
  search(board, &best_move, &stop_thinking, NULL, 5);
  char move_buf[16];
  move_to_string(best_move, move_buf);
  // printf("Found move: %s\n", move_buf);
  bool result;
  if (strings_equal(move_buf, second_move_buf)) {
    printf("CORRECT");
    result = true;
  } else {
    printf("WRONG");
    result = false;
  }
  printf(" rating: %s\n", cells[Rating]);
  //THREAD_JOIN(think_timer_thread, NULL);
  return result;
#undef FIELD_SIZE
#undef FIELD_COUNT
}

void puzzle_test(const char *puzzle_db_csv) {
  FILE *fp;
#define LINE_BUFFER_SIZE 2048
  char buffer[LINE_BUFFER_SIZE];
  fp = fopen(puzzle_db_csv, "r");
  if (fp == NULL) {
    printf("Error opening test case file.");
    return;
  }
  Board *board = calloc(1, sizeof(Board));
  int limit = 1000;
  fgets(buffer, LINE_BUFFER_SIZE, fp); // skip first line
  int i = 0;
  int correct = 0;
  int total = 0;
  while (fgets(buffer, LINE_BUFFER_SIZE, fp) && i < limit) {
    if (puzzle_test_line(buffer, board)) {
      correct++;
    }
    total++;
    i++;
  }
  printf("%i/%i puzzles correct first move (%0.2f percent)\n", correct, total,
         100.0 * (float)correct / (float)total);
  free(board);
#undef LINE_BUFFER_SIZE
}

bool eat_line_until_delim(const char *buffer, char *cell_buffer, char delimiter,
                          int *i) {
  int k = 0;
  while (1) {
    const char c = buffer[(*i)++];
    if (c == delimiter) {
      cell_buffer[k] = '\0';
      return true;
    } else if (c == '\0' || c == '\n') {
      cell_buffer[k] = '\0';
      return false;
    } else {
      cell_buffer[k++] = c;
    }
  }
}
