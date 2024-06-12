#include "chess.h"
#include "search.h"

f64 evaluate_material(Board *board);

#define FEATURE_COUNT 1

enum Features {
  kMaterialFeature,
};

// Scale is centipawns
f64 evaluation(Board *board, i32 side) {
  f64 features[FEATURE_COUNT];
  f64 weights[FEATURE_COUNT];

  features[kMaterialFeature] = evaluate_material(board);
  weights[kMaterialFeature] = 1.0;

  f64 score = 0;
  for (int i = 0; i < FEATURE_COUNT; i++) {
    score += features[i] * weights[i];
  }
  return side == kWhite ? score : -score;
}

#undef FEATURE_COUNT

f64 evaluate_material(Board *board) {
  static const f64 material_table[8] = {0,     0,     100.0, 300.0,
                                        300.0, 500.0, 900.0, 0};
  f64 material[2] = {0, 0};
  for (int i = 2; i < 8; i++) {
    material[kWhite] += material_table[i] * popcount(board->_bitboard[kWhite] &
                                                     board->_bitboard[i]);
    material[kBlack] += material_table[i] * popcount(board->_bitboard[kBlack] &
                                                     board->_bitboard[i]);
  }
  return material[kWhite] - material[kBlack];
}
