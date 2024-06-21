#include "chess.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

typedef struct TestHashMapElement {
  bool occupied;
  u64 hash;
  Board board;
} TestHashMapElement;

typedef struct TestHashMap {
  u64 count;
  TestHashMapElement *elements;
} TestHashMap;

bool boards_equal(Board *b1, Board *b2);

void simple_perft(Board *board, int depth, TestHashMap *map);

#define MASK 0xffff
void hashing_test() {
  const u64 hashmap_count = (u64)1 << 16;
  const u64 hashmap_size_bytes = sizeof(TestHashMapElement) * hashmap_count;
  const u64 mask = MASK;
  printf("Mask: %lu\n", mask);
  printf("Max element count: %lu\n", hashmap_count);
  printf("Hashmap size (bytes): %lu\n", hashmap_size_bytes);
  TestHashMap map;
  map.count = hashmap_count;
  map.elements = calloc(1, hashmap_size_bytes);
  Board *board = calloc(1, sizeof(Board));
  // board_initialize_startpos(board);
  // startpos
  //"r2qk1nr/pBp2ppp/3p1b2/4p3/4P3/2NP1Q1P/RPP2PP1/2B1K2R b Kkq - 0 10",
  //// castle
  //"r1bqkbnr/p1p1p1pp/np6/3pPp2/3P4/5N2/PPP2PPP/RNBQKB1R w KQkq f6 0 5",
  ////EP
  board_initialize_fen(
      board,
      "r1bqkbnr/p1p1p1pp/np6/3pPp2/3P4/5N2/PPP2PPP/RNBQKB1R w KQkq f6 0 5",
      NULL);
  simple_perft(board, 5, &map);
  // We've confirmed correct for perft(startpos, 5).
  // TODO: try testing with more dynamic position.
  printf("done perft\n");
  free(board);
  free(map.elements);
}

bool boards_equal(Board *b1, Board *b2) {
  bool bitboards_eq =
      memcmp(b1->_bitboard, b2->_bitboard, sizeof(u64) * 8) == 0;
  BoardMetadata *md1 = board_metadata_peek(b1, 0);
  BoardMetadata *md2 = board_metadata_peek(b2, 0);
  bool md_ep_eq = board_metadata_get_en_passant_square(md1) ==
                  board_metadata_get_en_passant_square(md2);
  bool md_c_eq = board_metadata_get_castling_rights(md1) ==
                 board_metadata_get_castling_rights(md2);
  bool same_turn = b1->_turn == b2->_turn;
  return bitboards_eq && md_ep_eq && md_c_eq && same_turn;
}

void simple_perft(Board *board, int depth, TestHashMap *map) {
  const u64 mask = MASK;
  const u64 board_hash = board_metadata_peek(board, 0)->_hash;
  TestHashMapElement *elem = &map->elements[board_hash & mask];
  if (!elem->occupied) {
    elem->board = *board;
    elem->hash = board_hash;
    elem->occupied = true;
  } else {
    if (elem->hash == board_hash) {
      if (boards_equal(&elem->board, board)) {
        printf("Collision Detected and Boards Equal.\n");
      } else {
        printf("Collision Detected and BOARD MISMATCH. Either hash collision "
               "or mistake in Zobrist.\n");
        board_dump(board);
        board_dump(&elem->board);
        getchar();
      }
    }
  }
  if (depth == 0)
    return;
  MoveList moves = generate_all_legal_moves(board);
  for (int i = 0; i < moves.count; i++) {
    Move mv = move_list_get(&moves, i);
    board_make_move(board, mv);
    simple_perft(board, depth - 1, map);
    board_unmake(board);
  }
}
#undef MASK
