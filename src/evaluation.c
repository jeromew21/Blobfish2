#include "chess.h"
#include "search.h"

typedef f64 (*evaluate_function)(Board *, i32);

f64 evaluate_material(Board *board, i32 side);

f64 evaluate_bishop_mobility(Board *board, i32 side);

f64 evaluate_knight_mobility(Board *board, i32 side);

f64 evaluate_king_safety(Board *board, i32 side);

#define FEATURE_COUNT 3

Centipawns evaluation(Board *board, i32 side) {
    // Scale is centipawns
    // 1 pawn is 100 cp
    // 100 cp is +0.1
    static const evaluate_function eval_functions[FEATURE_COUNT] = {
            evaluate_material,
            evaluate_bishop_mobility,
            evaluate_knight_mobility,
    };
    static const f64 weights[FEATURE_COUNT] = {
            1.0,
            1.0,
            1.0,
    }; // These might actually want to change based on game state but keep static const for now.
    // TODO check for terminal board state

    f64 features[FEATURE_COUNT];
    for (int i = 0; i < FEATURE_COUNT; i++) {
        features[i] = eval_functions[i](board, kWhite) - eval_functions[i](board, kBlack);
    }
    f64 score = 0;
    for (int i = 0; i < FEATURE_COUNT; i++) {
        score += features[i] * weights[i];
    }
    Centipawns cp_score = (Centipawns) score;
    return side == kWhite ? cp_score : -cp_score;
}

#undef FEATURE_COUNT

f64 evaluate_bishop_mobility(Board *board, i32 side) {
    const u64 *bb = board->_bitboard;
    const u64 occupancy_mask = bb[kWhite] | bb [kBlack];
    f64 mobility = 0;
    u64 bishops = (bb[kBishop] | bb[kQueen]) & bb[side];
    while (bishops) {
        u32 bishop_idx = bitscan_forward(bishops);
        // For sliding pieces, we can add times they attack own pieces (i.e.) protect.
        mobility += pop_count(bishop_moves(bishop_idx, occupancy_mask));
        bishops ^= (u64) 1 << bishop_idx;
    }
    return mobility;
}

f64 evaluate_knight_mobility(Board *board, i32 side) {
    const u64 *bb = board->_bitboard;
    f64 mobility = 0;
    u64 knights = bb[kKnight] & bb[side];
    while (knights) {
        u32 idx = bitscan_forward(knights);
        // For sliding pieces, we can add times they attack own pieces (i.e.) protect.
        mobility += pop_count(knight_moves(idx));
        knights ^= (u64) 1 << idx;
    }
    return mobility;
}

f64 evaluate_material(Board *board, i32 side) {
    static const f64 material_table[8] = {0, 0, 100.0, 300.0,
                                          300.0, 500.0, 900.0, 0};
    f64 material = 0;
    for (int i = 2; i < 8; i++) {
        material += material_table[i] * pop_count(board->_bitboard[side] &
                                                          board->_bitboard[i]);
    }
    return material;
}
