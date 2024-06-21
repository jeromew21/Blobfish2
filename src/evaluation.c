#include "chess.h"
#include "search.h"
#include <assert.h>

typedef f64 (*evaluate_function)(Board *, i32);

f64 max(f64 a, f64 b) { return a > b ? a : b; }

f64 min(f64 a, f64 b) { return a < b ? a : b; }

f64 evaluate_material(Board *board, i32 side);

f64 evaluate_bishop_mobility(Board *board, i32 side);

f64 evaluate_knight_mobility(Board *board, i32 side);

f64 evaluate_pawn_mobility(Board *board, i32 side);

f64 evaluate_king_safety(Board *board, i32 side);

i32 board_status(Board *board) {
  BoardMetadata *md = board_metadata_peek(board, 0);
  if (md->_is_repetition || (md->_halfmove_counter >= 100)) {
    return kDraw;
  }
  int legal_count = board_legal_moves_count(board);
  if (legal_count == 0) {
    bool check = board_is_check(board);
    if (check) {
      return kCheckmate;
    }
    return kStalemate;
  }
  return kPlayOn;
}

Centipawns evaluation(Board *board, i32 side) {
#define FEATURE_COUNT 5
  // Scale is centipawns
  // 1 pawn = 100 cp = +1
  // 1/10 of pawn = 10 cp = +0.1
  // 1/100 of pawn = 1 cp = +0.01
  static const evaluate_function eval_functions[FEATURE_COUNT] = {
      evaluate_material,        evaluate_bishop_mobility,
      evaluate_knight_mobility, evaluate_pawn_mobility,
      evaluate_king_safety,
  };
  static const f64 weights[FEATURE_COUNT] = {
      1.0, 20.0, 20.0, 20.0, 1.0,
  }; // These might actually want to change based
     // on game state but keep static const for now.
  f64 features[FEATURE_COUNT];
  for (int i = 0; i < FEATURE_COUNT; i++) {
    features[i] =
        eval_functions[i](board, kWhite) - eval_functions[i](board, kBlack);
  } // TODO: helper return eval vector so we can interpret
  f64 score = 0;
  for (int i = 0; i < FEATURE_COUNT; i++) {
    score += features[i] * weights[i];
  }
  Centipawns cp_score = (Centipawns)score;
  return side == kWhite ? cp_score : -cp_score;
#undef FEATURE_COUNT
}

f64 evaluate_bishop_mobility(Board *board, i32 side) {
  const u64 *bb = board->_bitboard;
  const u64 occupancy_mask = bb[kWhite] | bb[kBlack];
  f64 mobility = 0;
  u64 bishops = bb[kBishop] & bb[side];
  i32 bishop_count = pop_count(bishops);
  while (bishops) {
    u32 bishop_idx = bitscan_forward(bishops);
    // For sliding pieces, we can add times they attack own pieces (i.e.)
    // protect.
    mobility += pop_count(bishop_moves(bishop_idx, occupancy_mask));
    bishops ^= (u64)1 << bishop_idx;
  }
  if (bishop_count == 0)
    return 0;
  return mobility / (f64)bishop_count;
}

f64 evaluate_knight_mobility(Board *board, i32 side) {
  const u64 *bb = board->_bitboard;
  f64 mobility = 0;
  u64 knights = bb[kKnight] & bb[side];
  i32 knight_count = pop_count(knights);
  while (knights) {
    u32 idx = bitscan_forward(knights);
    // For sliding pieces, we can add times they attack own pieces (i.e.)
    // protect.
    mobility += pop_count(knight_moves(idx));
    knights ^= (u64)1 << idx;
  }
  if (knight_count == 0)
    return 0;
  return mobility / (f64)knight_count;
}

f64 evaluate_material(Board *board, i32 side) {
  static const f64 material_table[8] = {0,     0,     100.0, 300.0,
                                        300.0, 500.0, 900.0, 0};
  f64 material = 0;
  for (int i = 2; i < 8; i++) {
    material += material_table[i] *
                pop_count(board->_bitboard[side] & board->_bitboard[i]);
  }
  return material;
}

f64 evaluate_king_safety(Board *board, i32 side) {
  if (board->_bitboard[!side] & board->_bitboard[kQueen]) {
    const u64 king = board->_bitboard[side] & board->_bitboard[kKing];
    const u32 king_idx = bitscan_forward(king);
    const u64 rank1 = 0x00000000000000FF;
    const u64 rank8 = 0xFF00000000000000;
    const u64 rank = side == kWhite ? rank1 : rank8;
    // maybe make a 64 bit table of king positions?
    // corner = better, center = worse
    const bool on_back_rank = king & rank;
    u64 pawn_shield = king_moves(king_idx) & pawn_attacks(king, side) &
                      pawn_forward_moves(king, side) & board->_bitboard[side] &
                      board->_bitboard[kPawn];
    f64 pawn_shield_count = pop_count(pawn_shield);
    assert(pawn_shield_count <= 3);
    if (on_back_rank) {
      return 100.0 * pawn_shield_count;
    } else {
      return 10.0 * pawn_shield_count;
    }
  } else {
    // no enemy queen
    return 150.0;
  }
}

f64 evaluate_pawn_mobility(Board *board, i32 side) {
  const u64 occupancy_mask = board->_bitboard[side] & board->_bitboard[!side];
  const u64 pawns = board->_bitboard[side] & board->_bitboard[kPawn];
  const f64 pawn_count = pop_count(pawns);
  const f64 mobility =
      (f64)pop_count(pawn_forward_moves(pawns, side) & ~occupancy_mask) /
      pawn_count;
  return mobility;
}
