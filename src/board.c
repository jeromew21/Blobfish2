#include "bitboard_constants.h"
#include "chess.h"
#include <assert.h>

/**
 * Create board with basic start position.
 */
void board_initialize_startpos(Board *board) {
  board_initialize_fen(
      board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", NULL);
}


//given board, generate fen
//given board, generate sparse vector representation

/**
 * Given a board state, return the hash of board's current position.
 */
u64 board_position_hash(Board *board) {
  // zobrist index: (squares)*(piece-side) + (black to play?) + castling + en
  // passant file
  //              = 64*12 + 1 + 4 + 8 = 718
  u64 hash = 0;
  for (int i = 2; i < 8; i++) {
    u64 pieces = board->_bitboard[i];
    while (pieces) {
      u32 piece_idx = bitscan_forward(pieces);
      u64 piece_bitset = (u64)1 << piece_idx;
      i32 color = (piece_bitset & board->_bitboard[kWhite]) ? kWhite : kBlack;
      hash ^= zobrist_key(i, piece_idx, color);
      pieces ^= piece_bitset;
    }
  }
  if (board->_turn == kBlack) {
    hash ^= ZOBRIST_KEYS[ZOBRIST_BLACK_TO_MOVE];
  }
  BoardMetadata *md = board_metadata_peek(board, 0);
  u32 castling_rights = board_metadata_get_castling_rights(md);
  for (int i = 0; i < 4; i++) {
    u32 right = ((u32)1 << i) & castling_rights;
    if (right) {
      hash ^= ZOBRIST_KEYS[ZOBRIST_CASTLING + i];
    }
  }
  u32 ep_square = board_metadata_get_en_passant_square(md);
  if (ep_square > 0) {
    const i32 col = (i32)ep_square % 8;
    hash ^= ZOBRIST_KEYS[ZOBRIST_EN_PASSANT + col];
  }
  return hash;
}

u64 zobrist_key(i32 piece, u32 square, i32 color) {
  i32 base_offset = (piece - 2) * 128;
  i32 k = (base_offset + (i32)square) + (64 * color);
  return ZOBRIST_KEYS[k];
}

BoardMetadata *board_metadata_peek(Board *board, int i) {
  i32 idx = board->_ply - 1 - i;
  assert(idx >= 0);
  return &board->_state_stack[idx];
}

void board_metadata_set_en_passant_square(BoardMetadata *md, u32 ep_square) {
  md->_state_data &= ~(0xff << 8);
  md->_state_data |= (ep_square & 0xff) << 8;
}

void board_metadata_set_captured_piece(BoardMetadata *md, u32 captured_piece) {
  md->_state_data &= ~(0xf << 4);
  md->_state_data |= (captured_piece & 0xf) << 4;
}

void board_metadata_set_castling_rights(BoardMetadata *md, u32 rights) {
  md->_state_data &= ~(0xf); // is needed?
  md->_state_data |= (rights & 0xf);
}

u32 board_metadata_get_en_passant_square(BoardMetadata *md) {
  return (md->_state_data >> 8) & 0xff;
}

u32 board_metadata_get_captured_piece(BoardMetadata *md) {
  return (md->_state_data >> 4) & 0xf;
}

u32 board_metadata_get_castling_rights(BoardMetadata *md) {
  return md->_state_data & 0xf;
}
