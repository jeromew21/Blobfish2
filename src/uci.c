#include "uci.h"
#include "chess.h"
#include "perft.h"
#include "search.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// idk man...
#include <pthread.h>

EngineContext *ctx;

Board *board;

typedef void (*_cmd_fn)(char *);

bool is_alphanum(const char c) { return isalpha(c) || isdigit(c); }

bool is_whitespace(const char c) { return c == ' ' || c == '\t'; }

bool strings_equal(char *a, char *b) { return strcmp(a, b) == 0; }

void eat_whitespace(char *line_buffer, int *i);

bool eat_word(char *line_buffer, char *word_buffer, int *i);

bool board_make_move_from_alg(Board *board, const char *algebraic);

void move_to_string(Move mv, char *buf);

void command_uci(char *line_buffer);

void command_debug(char *line_buffer);

void command_isready(char *line_buffer);

void command_setoption(char *line_buffer);

void command_go(char *line_buffer);

void command_stop(char *line_buffer);

void command_perft(char *line_buffer);

void command_ucinewgame(char *line_buffer);

void command_position(char *line_buffer);

void command_quit(char *line_buffer);

void engine_command(char *line_buffer) {
#define COMMAND_COUNT 8
  static char *commands[COMMAND_COUNT] = {"quit",     "uci",        "perft",
                                          "position", "ucinewgame", "isready",
                                          "go",       "stop"};
  static const _cmd_fn command_functions[COMMAND_COUNT] = {
      command_quit,       command_uci,     command_perft, command_position,
      command_ucinewgame, command_isready, command_go,    command_stop};
  char command_name[64];
  int i = 0;
  eat_whitespace(line_buffer, &i);
  eat_word(line_buffer, command_name, &i);
  for (int k = 0; k < COMMAND_COUNT; k++) {
    if (strings_equal(command_name, commands[k])) {
      command_functions[k](line_buffer + i);
      break;
    }
  }
#undef COMMAND_COUNT
}

void eat_whitespace(char *line_buffer, int *i) {
  while (1) {
    const char c = line_buffer[*i];
    if (!is_whitespace(c)) {
      break;
    }
    (*i)++;
  }
}

/**
 * Move the pointer along, filling word_buffer with the first found word, null
 * terminated. Consume all following whitespace.
 * Returns false if there isn't another word to eat (i.e. null terminator.)
 */
bool eat_word(char *line_buffer, char *word_buffer, int *i) {
  int k = 0;
  while (1) {
    const char c = line_buffer[*i];
    if (is_whitespace(c)) {
      break;
    }
    if (c == '\n') {
      if (k == 0) // first character we're seeing is a newline
        return false;
      else
        break;
    }
    word_buffer[k++] = c;
    (*i)++;
  }
  word_buffer[k] = '\0';
  eat_whitespace(line_buffer, i);
  return true;
}

void *think(void *foo) {
  search(board, &ctx->stop_thinking,
         &ctx->best_move); // add engine context to params?
  printf("info finished searching\n");
  char move_buf[16];
  move_to_string(ctx->best_move, move_buf);
  printf("bestmove %s\n", move_buf);
  return NULL;
}

void command_go(char *line_buffer) {
  ctx->stop_thinking = false;
  pthread_create(&ctx->think_thread, NULL, think, (void *)NULL);
}

void command_stop(char *line_buffer) {
  ctx->stop_thinking = true;
  pthread_join(ctx->think_thread, NULL);
}

void command_perft(char *line_buffer) {
  // TODO: perft command
  // printf("perft %s\n", line_buffer);
}

void command_ucinewgame(char *line_buffer) {
  // not sure this is meaningful for us
}

void command_isready(char *line_buffer) { printf("readyok\n"); }

void command_quit(char *line_buffer) { ctx->quit = true; }

void command_position(char *line_buffer) {
  int i = 0;
  memset(board, 0, sizeof(Board));
  char word[16];
  eat_word(line_buffer, word, &i);
  if (strings_equal("fen", word)) {
    fen_parse(board, line_buffer, &i);
  } else {
    board_initialize_startpos(board);
  }
  eat_word(line_buffer, word, &i);
  if (strings_equal("moves", word)) {
    while (eat_word(line_buffer, word, &i)) {
      board_make_move_from_alg(board, word);
    }
  }
}

void command_uci(char *line_buffer) {
  printf("id name Blobfish 0.0\n");
  printf("id author Jerome Wei\n");
  printf("option name Foo type check default false\n");
  // print options
  printf("uciok\n");
}

EngineContext *engine_get_context() { return ctx; }

void engine_initialize() {
  board = board_uninitialized();
  board_initialize_startpos(board);
  ctx = malloc(sizeof(EngineContext));
  ctx->debug = false;
  ctx->quit = false;
}

void engine_cleanup() {
  free(ctx);
  free(board);
}

/**
 * Given an algebraic move like "a2b1q", make the move on board.
 * This shouldn't be called within a loop.
 * TODO: promotions.
 */
bool board_make_move_from_alg(Board *board, const char *algebraic) {
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
    return false;
  }
  board_make_move(board, mv);
  return true;
}

// TODO: fix this
void move_to_string(Move mv, char *buf) {
  // returns string of length 5, i.e. e2-e4
  static char row_names[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
  static char col_names[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
  u32 src = move_get_src_u32(mv);
  u32 dest = move_get_dest_u32(mv);
  // printf("%c%c-%c%c", col_names[src%8], row_names[src/8],
  // col_names[dest%8], row_names[dest/8]);
  char special[256];
  special[0] = '\0';
  u32 md = move_get_metadata(mv);
  if (md == kEnPassantMove) {
    // sprintf(special, "%s", " en passant");
  } else if (md == kKingSideCastleMove) {
    // sprintf(special, "%s", " O-O");
  } else if (md == kQueenSideCastleMove) {
    // sprintf(special, "%s", " O-O-O");
  } else if (md & 0x8) {
    // TODO: underpromotions
    sprintf(special, "%s", "q");
  }
  sprintf(buf, "%c%c%c%c%s", col_names[src % 8], row_names[src / 8],
          col_names[dest % 8], row_names[dest / 8], special);
}
