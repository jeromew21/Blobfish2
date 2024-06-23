#include "chess.h"
#include "search.h"
#include <assert.h>

#define FEATURE_COUNT 5

typedef struct EvaluationVector {
  Centipawns features[FEATURE_COUNT];
} EvaluationVector;

typedef f64 (*evaluate_function)(Board *, i32);

/*
f64 maxf(f64 a, f64 b) { return a > b ? a : b; }

f64 minf(f64 a, f64 b) { return a < b ? a : b; }
*/

f64 evaluate_material(Board *board, i32 color);

f64 evaluate_bishop_mobility(Board *board, i32 color);

f64 evaluate_rook_mobility(Board *board, i32 color);

f64 evaluate_knight_mobility(Board *board, i32 color);

f64 evaluate_pawn_mobility(Board *board, i32 color);

f64 evaluate_king_pawn_shield(Board *board, i32 color);

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

// Note: we've changed it so that eval no longer takes into account terminal
// board states. It should just evaluate checkmate as bad.
// NOTE: Be extremely careful about division by zero. It's bitten me again and
// again. We might even try doing a safe division function.
EvaluationVector evaluation_vector(Board *board) {
  // Scale is centipawns
  // 1000 pawn = 100000 = +1000
  // 100 pawn = 10000 = +100
  // 10 pawn = 1000 cp = +10
  // 1 pawn = 100 cp = +1
  // 1/10 of pawn = 10 cp = +0.1
  // 1/100 of pawn = 1 cp = +0.01
  // And so on and so on
  static const evaluate_function eval_functions[FEATURE_COUNT] = {
      evaluate_material,         evaluate_bishop_mobility,
      evaluate_knight_mobility,  evaluate_rook_mobility,
      evaluate_king_pawn_shield,
  };
  // on game state but keep static const for now.
  EvaluationVector vec;
  for (int i = 0; i < FEATURE_COUNT; i++) {
    vec.features[i] =
        eval_functions[i](board, kWhite) - eval_functions[i](board, kBlack);
  }
  return vec;
}

Centipawns evaluation(Board *board) {
  static const f64 weights[FEATURE_COUNT] = {
      1.0, 1.0, 1.0, 1.0, 1.0,
  };
  EvaluationVector vec = evaluation_vector(board);
  f64 score = 0;
  for (int i = 0; i < FEATURE_COUNT; i++) {
    score += vec.features[i] * weights[i];
  }
  Centipawns cp_score = (Centipawns)score;
  return board->_turn == kWhite ? cp_score : -cp_score;
}
#undef FEATURE_COUNT

f64 evaluate_bishop_mobility(Board *board, i32 color) {
  const u64 *bb = board->_bitboard;
  const u64 occupancy_mask = bb[kWhite] | bb[kBlack];
  f64 mobility = 0;
  u64 bishops = bb[kBishop] & bb[color];
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

f64 evaluate_rook_mobility(Board *board, i32 color) {
  const u64 *bb = board->_bitboard;
  const u64 occupancy_mask = bb[kWhite] | bb[kBlack];
  f64 mobility = 0;
  u64 rooks = bb[kRook] & bb[color];
  i32 rook_count = pop_count(rooks);
  while (rooks) {
    u32 rook_idx = bitscan_forward(rooks);
    // For sliding pieces, we can add times they attack own pieces (i.e.)
    // protect.
    mobility += pop_count(rook_moves(rook_idx, occupancy_mask));
    rooks ^= (u64)1 << rook_idx;
  }
  if (rook_count == 0)
    return 0;
  return mobility / (f64)rook_count;
}

f64 evaluate_knight_mobility(Board *board, i32 color) {
  const u64 *bb = board->_bitboard;
  f64 mobility = 0;
  u64 knights = bb[kKnight] & bb[color];
  i32 knight_count = pop_count(knights);
  while (knights) {
    u32 idx = bitscan_forward(knights);
    mobility += pop_count(knight_moves(idx));
    knights ^= (u64)1 << idx;
  }
  if (knight_count == 0)
    return 0;
  return mobility / (f64)knight_count;
}

f64 evaluate_material(Board *board, i32 color) {
  static const f64 material_table[8] = {0,     0,     100.0, 301.0,
                                        299.0, 500.0, 900.0, 0};
  f64 material = 0;
  for (int i = 2; i < 8; i++) {
    material += material_table[i] *
                pop_count(board->_bitboard[color] & board->_bitboard[i]);
  }
  return material;
}

f64 evaluate_king_pawn_shield(Board *board, i32 color) {
  const u64 king = board->_bitboard[color] & board->_bitboard[kKing];
  const u32 king_idx = bitscan_forward(king);
  const u64 rank1 = 0x00000000000000FF;
  const u64 rank8 = 0xFF00000000000000;
  const u64 a_file = 0x0101010101010101;
  const u64 h_file = 0x8080808080808080;
  const u64 rank = color == kWhite ? rank1 : rank8;
  // maybe make a 64 bit table of king positions?
  // corner = better, center = worse
  const bool on_back_rank = king & rank;
  u64 pawn_shield = king_moves(king_idx) & pawn_attacks(king, color) &
                    pawn_forward_moves(king, color) & board->_bitboard[color] &
                    board->_bitboard[kPawn];
  f64 pawn_shield_count = pop_count(pawn_shield);
  assert(pawn_shield_count <= 3);
  if (on_back_rank) {
    if (king & a_file || king & h_file) {
      return 150.0 * pawn_shield_count; // 0, 150, 300
    }
    return 100.0 * pawn_shield_count; // 0, 100, 200, 300
  } else {
    return 0.0;
  }
}

f64 evaluate_pawn_mobility(Board *board, i32 color) {
  const u64 occupancy_mask = board->_bitboard[color] & board->_bitboard[!color];
  const u64 pawns = board->_bitboard[color] & board->_bitboard[kPawn];
  const f64 pawn_count = pop_count(pawns);
  const f64 mobility =
      (f64)pop_count(pawn_forward_moves(pawns, color) & ~occupancy_mask) /
      pawn_count; // DIV BY ZERO??
  return mobility;
}
