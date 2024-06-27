#include "bitboard_constants.h"
#include "chess.h"
#include <stdbool.h>
#include <stdlib.h>

/*Intrinsics not working on Mac
    TODO: try using on Linux
*/
/*#include <immintrin.h>*/

typedef u32 (*bitscan_function)(u64);

MoveList generate_all_pseudo_legal_moves(Board *board);

u32 pop_lsb();

/**
 * TODO: optimize
 * this is an important bottleneck in qsearch
 */
MoveList generate_capture_moves_old(Board *board) {
  MoveList captures = move_list_create();
  MoveList legal = generate_all_legal_moves(board);
  for (int i = 0; i < legal.count; i++) {
    Move mv = move_list_get(&legal, i);
    u32 md = move_get_metadata(mv);
    if (md & CAPTURE_BIT_FLAG) {
      move_list_push(&captures, mv);
    }
  }
  return captures;
}

MoveList generate_capture_moves(Board *board) {
    MoveList move_list = move_list_create();
    const u64 friendly_mask = board->_bitboard[board->_turn];
    const u64 enemy_mask = board->_bitboard[!board->_turn];
    const u64 occupancy_mask = friendly_mask | enemy_mask;
    // Knights
    // https://www.chessprogramming.org/Knight_Pattern
    // TODO: use conditional AVX instructions if possible
    // hitting static data might be slow, idk
    // Or even just shifting/adding offset
    // must measure
    u64 knights = friendly_mask & board->_bitboard[kKnight];
    while (knights) {
        const u32 idx = bitscan_forward(knights);
        u64 jumps = knight_moves(idx) & enemy_mask;
        while (jumps) {
            const u32 dest_idx = bitscan_forward(jumps);
            const u64 dest_bit = (u64)1 << dest_idx;
            const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
            const Move mv = move_create(idx, dest_idx, md);
            move_list_push(&move_list, mv);
            jumps ^= dest_bit;
        }
        knights ^= (u64)1 << idx;
    }
    const u64 king = friendly_mask & board->_bitboard[kKing];
    // Same note as for Knights w.r.t. optimization
    if (king) { // technically this check isn't needed for legal positions
        const u32 king_idx = bitscan_forward(king);
        u64 king_move_bitset = king_moves(king_idx) & enemy_mask;
        while (king_move_bitset) {
            const u32 dest_idx = bitscan_forward(king_move_bitset);
            const u64 dest_bit = (u64)1 << dest_idx;
            const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
            const Move mv = move_create(king_idx, dest_idx, md);
            move_list_push(&move_list, mv);
            king_move_bitset ^= dest_bit;
        }
    }
    // Pawns
    const u64 pawns = friendly_mask & board->_bitboard[kPawn];
    if (pawns) {
        const u64 not_a_file = (u64) ~0x0101010101010101;
        const u64 not_h_file = (u64) ~0x8080808080808080;
        const u64 last_rank = 0xFF000000000000FF;
        const u32 ep_idx =
                board_metadata_get_en_passant_square(board_metadata_peek(board, 0));
        const u64 en_passant_square = ep_idx == 0 ? 0 : (u64)1 << ep_idx;
        u64 pawn_east_attacks;
        u64 pawn_west_attacks;
        // push
        i32 east_offset;
        i32 west_offset;
        if (board->_turn == kWhite) {
            pawn_east_attacks =
                    ((pawns << 9) & not_a_file) & (en_passant_square | enemy_mask);
            pawn_west_attacks =
                    ((pawns << 7) & not_h_file) & (en_passant_square | enemy_mask);
            east_offset = -9;
            west_offset = -7;
        } else {
            pawn_east_attacks =
                    ((pawns >> 7) & not_a_file) & (en_passant_square | enemy_mask);
            pawn_west_attacks =
                    ((pawns >> 9) & not_h_file) & (en_passant_square | enemy_mask);
            east_offset = 7;
            west_offset = 9;
        }
        while (pawn_east_attacks) {
            const u32 dest_idx = bitscan_forward(pawn_east_attacks);
            const u32 src_idx = ((i32)dest_idx) + east_offset;
            const u64 dest_bit = (u64)1 << dest_idx;
            if (dest_bit & last_rank) {
                move_list_push(&move_list, move_create(src_idx, dest_idx,
                                                       kQueenCapturePromotionMove));
                move_list_push(&move_list, move_create(src_idx, dest_idx,
                                                       kBishopCapturePromotionMove));
                move_list_push(&move_list, move_create(src_idx, dest_idx,
                                                       kKnightCapturePromotionMove));
                move_list_push(&move_list, move_create(src_idx, dest_idx,
                                                       kRookCapturePromotionMove));
            } else {
                const u32 flags =
                        (dest_bit & en_passant_square) ? kEnPassantMove : kCaptureMove;
                move_list_push(&move_list, move_create(src_idx, dest_idx, flags));
            }
            pawn_east_attacks ^= dest_bit;
        }
        while (pawn_west_attacks) {
            const u32 dest_idx = bitscan_forward(pawn_west_attacks);
            const u32 src_idx = ((i32)dest_idx) + west_offset;
            const u64 dest_bit = (u64)1 << dest_idx;
            if (dest_bit & last_rank) {
                move_list_push(&move_list, move_create(src_idx, dest_idx,
                                                       kQueenCapturePromotionMove));
                move_list_push(&move_list, move_create(src_idx, dest_idx,
                                                       kBishopCapturePromotionMove));
                move_list_push(&move_list, move_create(src_idx, dest_idx,
                                                       kKnightCapturePromotionMove));
                move_list_push(&move_list, move_create(src_idx, dest_idx,
                                                       kRookCapturePromotionMove));
            } else {
                u32 flags =
                        (dest_bit & en_passant_square) ? kEnPassantMove : kCaptureMove;
                move_list_push(&move_list, move_create(src_idx, dest_idx, flags));
            }
            pawn_west_attacks ^= dest_bit;
        }
    }
    u64 bishops =
            friendly_mask & (board->_bitboard[kBishop] | board->_bitboard[kQueen]);
    while (bishops) {
        const u32 src_idx = bitscan_forward(bishops);
        u64 destinations = bishop_moves(src_idx, occupancy_mask) & enemy_mask;
        while (destinations) {
            const u32 dest_idx = bitscan_forward(destinations);
            const u64 dest_bit = (u64)1 << dest_idx;
            const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
            const Move mv = move_create(src_idx, dest_idx, md);
            move_list_push(&move_list, mv);
            destinations ^= dest_bit;
        }
        bishops ^= (u64)1 << src_idx;
    }
    u64 rooks =
            friendly_mask & (board->_bitboard[kRook] | board->_bitboard[kQueen]);
    while (rooks) {
        const u32 src_idx = bitscan_forward(rooks);
        u64 destinations = rook_moves(src_idx, occupancy_mask) & enemy_mask;
        while (destinations) {
            const u32 dest_idx = bitscan_forward(destinations);
            const u64 dest_bit = (u64)1 << dest_idx;
            const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
            const Move mv = move_create(src_idx, dest_idx, md);
            move_list_push(&move_list, mv);
            destinations ^= dest_bit;
        }
        rooks ^= (u64)1 << src_idx;
    }
    MoveList legal = move_list_create();
    for (int i = 0; i < move_list.count; i++) {
        u64 bitboards[8];
        for (int k = 0; k < 8; k++) { // could be memcpy instead
            bitboards[k] = board->_bitboard[k];
        }
        Move mv = move_list_get(&move_list, i);
        bitboards_update(bitboards, board->_turn, mv);
        if (!is_attacked(bitboards[board->_turn] & bitboards[kKing], bitboards,
                         !board->_turn)) {
            move_list_push(&legal, mv);
        }
    }
    return legal;
}

/*
 * TODO: optimize
 */
i32 board_legal_moves_count(Board *board) {
  MoveList legal = generate_all_legal_moves(board);
  return legal.count;
}

/**
 * In the future, we might generate unchecking and/or special moves separately.
 */
MoveList generate_all_legal_moves(Board *board) {
  MoveList legal = move_list_create();
  MoveList pseudo_legal = generate_all_pseudo_legal_moves(board);
  for (int i = 0; i < pseudo_legal.count; i++) {
    u64 bitboards[8];
    for (int k = 0; k < 8; k++) { // could be memcpy instead
      bitboards[k] = board->_bitboard[k];
    }
    Move mv = move_list_get(&pseudo_legal, i);
    bitboards_update(bitboards, board->_turn, mv);
    if (!is_attacked(bitboards[board->_turn] & bitboards[kKing], bitboards,
                     !board->_turn)) {
      move_list_push(&legal, mv);
    }
  }
  return legal;
}

bool board_is_check(Board *board) {
  return is_attacked(board->_bitboard[board->_turn] & board->_bitboard[kKing],
                     board->_bitboard, !board->_turn);
}

MoveList generate_all_pseudo_legal_moves(Board *board) {
  MoveList move_list = move_list_create();
  const u64 friendly_mask = board->_bitboard[board->_turn];
  const u64 enemy_mask = board->_bitboard[!board->_turn];
  const u64 occupancy_mask = friendly_mask | enemy_mask;
  // Knights
  // https://www.chessprogramming.org/Knight_Pattern
  // TODO: use conditional AVX instructions if possible
  // hitting static data might be slow, idk
  // Or even just shifting/adding offset
  // must measure
  u64 knights = friendly_mask & board->_bitboard[kKnight];
  while (knights) {
    const u32 idx = bitscan_forward(knights);
    u64 jumps = knight_moves(idx) & ~friendly_mask;
    while (jumps) {
      const u32 dest_idx = bitscan_forward(jumps);
      const u64 dest_bit = (u64)1 << dest_idx;
      const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
      const Move mv = move_create(idx, dest_idx, md);
      move_list_push(&move_list, mv);
      jumps ^= dest_bit;
    }
    knights ^= (u64)1 << idx;
  }
  // Castling
  // TODO: generalized, 960 castling the rule is there's a king target square
  // for all positions Since castling is defined where the DEST of king+rook is
  // constant, we could encode the original king-original rook pos.
  const u64 king = friendly_mask & board->_bitboard[kKing];
  if (king) {
    BoardMetadata *md = board_metadata_peek(board, 0);
    const u32 castling_rights = board_metadata_get_castling_rights(md);
    if ((board->_turn == kWhite && !(castling_rights & kWhiteKingSideFlag)) ||
        (board->_turn == kBlack && !(castling_rights & kBlackKingSideFlag))) {
      const u32 king_dest = board->_turn == kWhite ? 6 : 62;
      // 960: need to fix this
      const u64 king_squares =
          king | (king << 1) | (king << 2); // these must all not be attacked
      const u64 must_be_empty_squares = king ^ king_squares;
      if (!(must_be_empty_squares & occupancy_mask)) {
        if (!is_attacked(king_squares, board->_bitboard, !board->_turn)) {
          const Move mv = move_create(bitscan_forward(king), king_dest,
                                      kKingSideCastleMove);
          move_list_push(&move_list, mv);
        }
      }
    }
    if ((board->_turn == kWhite && !(castling_rights & kWhiteQueenSideFlag)) ||
        (board->_turn == kBlack && !(castling_rights & kBlackQueenSideFlag))) {
      const u32 king_dest = board->_turn == kWhite ? 2 : 58;
      // 960: need to fix this
      const u64 king_squares = king | (king >> 1) | (king >> 2);
      const u64 must_be_empty_squares = king ^ (king_squares | (king >> 3));
      if (!(must_be_empty_squares & occupancy_mask)) {
        if (!is_attacked(king_squares, board->_bitboard, !board->_turn)) {
          const Move mv = move_create(bitscan_forward(king), king_dest,
                                      kQueenSideCastleMove);
          move_list_push(&move_list, mv);
        }
      }
    }
  }
  // King
  // Same note as for Knights w.r.t. optimization
  if (king) { // technically this check isn't needed for legal positions
    const u32 king_idx = bitscan_forward(king);
    u64 king_move_bitset = king_moves(king_idx) & ~friendly_mask;
    while (king_move_bitset) {
      const u32 dest_idx = bitscan_forward(king_move_bitset);
      const u64 dest_bit = (u64)1 << dest_idx;
      const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
      const Move mv = move_create(king_idx, dest_idx, md);
      move_list_push(&move_list, mv);
      king_move_bitset ^= dest_bit;
    }
  }
  // Pawns
  const u64 pawns = friendly_mask & board->_bitboard[kPawn];
  if (pawns) {
    const u64 empty_mask = ~occupancy_mask;
    const u64 not_a_file = (u64) ~0x0101010101010101;
    const u64 not_h_file = (u64) ~0x8080808080808080;
    const u64 last_rank = 0xFF000000000000FF;
    const u32 ep_idx =
        board_metadata_get_en_passant_square(board_metadata_peek(board, 0));
    const u64 en_passant_square = ep_idx == 0 ? 0 : (u64)1 << ep_idx;
    u64 pawn_east_attacks;
    u64 pawn_west_attacks;
    u64 pawn_jumps_single;
    u64 pawn_double_push_destinations; // locations of pawns that can double
    // push
    i32 east_offset;
    i32 west_offset;
    i32 pawn_jump_offset;
    if (board->_turn == kWhite) {
      const u64 fourth_rank = 0x00000000FF000000;
      pawn_east_attacks =
          ((pawns << 9) & not_a_file) & (en_passant_square | enemy_mask);
      pawn_west_attacks =
          ((pawns << 7) & not_h_file) & (en_passant_square | enemy_mask);
      pawn_jumps_single = (pawns << 8) & empty_mask;
      east_offset = -9;
      west_offset = -7;
      pawn_jump_offset = -8;
      pawn_double_push_destinations =
          (pawn_jumps_single << 8) & empty_mask & fourth_rank;
    } else {
      const u64 fifth_rank = 0x000000FF00000000;
      pawn_east_attacks =
          ((pawns >> 7) & not_a_file) & (en_passant_square | enemy_mask);
      pawn_west_attacks =
          ((pawns >> 9) & not_h_file) & (en_passant_square | enemy_mask);
      east_offset = 7;
      west_offset = 9;
      pawn_jump_offset = 8;
      pawn_jumps_single = (pawns >> 8) & empty_mask;
      pawn_double_push_destinations =
          (pawn_jumps_single >> 8) & empty_mask & fifth_rank;
    }
    while (pawn_east_attacks) {
      const u32 dest_idx = bitscan_forward(pawn_east_attacks);
      const u32 src_idx = ((i32)dest_idx) + east_offset;
      const u64 dest_bit = (u64)1 << dest_idx;
      if (dest_bit & last_rank) {
        move_list_push(&move_list, move_create(src_idx, dest_idx,
                                               kQueenCapturePromotionMove));
        move_list_push(&move_list, move_create(src_idx, dest_idx,
                                               kBishopCapturePromotionMove));
        move_list_push(&move_list, move_create(src_idx, dest_idx,
                                               kKnightCapturePromotionMove));
        move_list_push(&move_list, move_create(src_idx, dest_idx,
                                               kRookCapturePromotionMove));
      } else {
        const u32 flags =
            (dest_bit & en_passant_square) ? kEnPassantMove : kCaptureMove;
        move_list_push(&move_list, move_create(src_idx, dest_idx, flags));
      }
      pawn_east_attacks ^= dest_bit;
    }
    while (pawn_west_attacks) {
      const u32 dest_idx = bitscan_forward(pawn_west_attacks);
      const u32 src_idx = ((i32)dest_idx) + west_offset;
      const u64 dest_bit = (u64)1 << dest_idx;
      if (dest_bit & last_rank) {
        move_list_push(&move_list, move_create(src_idx, dest_idx,
                                               kQueenCapturePromotionMove));
        move_list_push(&move_list, move_create(src_idx, dest_idx,
                                               kBishopCapturePromotionMove));
        move_list_push(&move_list, move_create(src_idx, dest_idx,
                                               kKnightCapturePromotionMove));
        move_list_push(&move_list, move_create(src_idx, dest_idx,
                                               kRookCapturePromotionMove));
      } else {
        u32 flags =
            (dest_bit & en_passant_square) ? kEnPassantMove : kCaptureMove;
        move_list_push(&move_list, move_create(src_idx, dest_idx, flags));
      }
      pawn_west_attacks ^= dest_bit;
    }
    while (pawn_jumps_single) {
      const u32 dest_idx = bitscan_forward(pawn_jumps_single);
      const u32 src_idx = ((i32)dest_idx) + pawn_jump_offset;
      const u64 dest_bit = (u64)1 << dest_idx;
      if (dest_bit & last_rank) {
        move_list_push(&move_list,
                       move_create(src_idx, dest_idx, kQueenPromotionMove));
        move_list_push(&move_list,
                       move_create(src_idx, dest_idx, kKnightPromotionMove));
        move_list_push(&move_list,
                       move_create(src_idx, dest_idx, kBishopPromotionMove));
        move_list_push(&move_list,
                       move_create(src_idx, dest_idx, kRookPromotionMove));
      } else {
        move_list_push(&move_list, move_create(src_idx, dest_idx, kQuietMove));
      }
      pawn_jumps_single ^= dest_bit;
    }
    while (pawn_double_push_destinations) {
      const u32 dest_idx = bitscan_forward(pawn_double_push_destinations);
      const u32 src_idx = ((i32)dest_idx) + pawn_jump_offset + pawn_jump_offset;
      const Move mv = move_create(src_idx, dest_idx, kDoublePawnMove);
      move_list_push(&move_list, mv);
      pawn_double_push_destinations ^= (u64)1 << dest_idx;
    }
  }
  u64 bishops =
      friendly_mask & (board->_bitboard[kBishop] | board->_bitboard[kQueen]);
  while (bishops) {
    const u32 src_idx = bitscan_forward(bishops);
    u64 destinations = bishop_moves(src_idx, occupancy_mask) & ~friendly_mask;
    while (destinations) {
      const u32 dest_idx = bitscan_forward(destinations);
      const u64 dest_bit = (u64)1 << dest_idx;
      const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
      const Move mv = move_create(src_idx, dest_idx, md);
      move_list_push(&move_list, mv);
      destinations ^= dest_bit;
    }
    bishops ^= (u64)1 << src_idx;
  }
  u64 rooks =
      friendly_mask & (board->_bitboard[kRook] | board->_bitboard[kQueen]);
  while (rooks) {
    const u32 src_idx = bitscan_forward(rooks);
    u64 destinations = rook_moves(src_idx, occupancy_mask) & ~friendly_mask;
    while (destinations) {
      const u32 dest_idx = bitscan_forward(destinations);
      const u64 dest_bit = (u64)1 << dest_idx;
      const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
      const Move mv = move_create(src_idx, dest_idx, md);
      move_list_push(&move_list, mv);
      destinations ^= dest_bit;
    }
    rooks ^= (u64)1 << src_idx;
  }
  return move_list;
}

/**
 * Check if the squares marked by bitset are attacked by any pieces of
 * attacking_color. Should be more efficient than an attacker count, due to
 * possibility of early return. Or is it; not sure how efficient OR-ing all the
 * stuff together then branching once is.
 */
bool is_attacked(u64 bitset, u64 *bitboards, i32 attacking_color) {
  const u64 occupancy_mask = bitboards[kWhite] | bitboards[kBlack];
  const u64 enemy_mask = bitboards[attacking_color];
  while (bitset) {
    const u32 target_idx = bitscan_forward(bitset);
    const u64 target_bit = (u64)1 << target_idx;
    const u64 pawn_attackers =
        pawn_attacks(target_bit, !attacking_color) & bitboards[kPawn];
    const u64 knight_attackers = knight_moves(target_idx) & bitboards[kKnight];
    const u64 king_attackers = king_moves(target_idx) & bitboards[kKing];
    if (enemy_mask & (pawn_attackers | knight_attackers | king_attackers)) {
      return true;
    }
    if (bishop_moves(target_idx, occupancy_mask) & enemy_mask &
        (bitboards[kBishop] | bitboards[kQueen])) {
      return true;
    }
    if (rook_moves(target_idx, occupancy_mask) & enemy_mask &
        (bitboards[kRook] | bitboards[kQueen])) {
      return true;
    }
    bitset ^= target_bit;
  }
  return false;
}

i32 attacker_count(u64 bitset, u64 *bitboards, i32 attacking_color) {
  const u64 occupancy_mask = bitboards[kWhite] | bitboards[kBlack];
  const u64 enemy_mask = bitboards[attacking_color];
  u64 attackers = 0;
  while (bitset) {
    const u32 target_idx = bitscan_forward(bitset);
    const u64 target_bit = (u64)1 << target_idx;
    attackers |= pawn_attacks(target_bit, !attacking_color) & enemy_mask &
                 bitboards[kPawn];
    attackers |= knight_moves(target_idx) & (enemy_mask & bitboards[kKnight]);
    attackers |= king_moves(target_idx) & enemy_mask & bitboards[kKing];
    attackers |= bishop_moves(target_idx, occupancy_mask) & enemy_mask &
                 (bitboards[kBishop] | bitboards[kQueen]);
    attackers |= rook_moves(target_idx, occupancy_mask) & enemy_mask &
                 (bitboards[kRook] | bitboards[kQueen]);
    bitset ^= target_bit;
  }
  return pop_count(attackers);
}

/**
 * note this requires bitset instead of flat index
 * TODO: verify this works for *entire* pawn set and not only single bitset?
 */
u64 pawn_attacks(u64 source_bitset, i32 side) {
  const u64 not_a_file = (u64)~0x0101010101010101;
  const u64 not_h_file = (u64)~0x8080808080808080;
  if (side == kWhite) {
    return ((source_bitset << 9) & not_a_file) |
           ((source_bitset << 7) & not_h_file);
  } else {
    return ((source_bitset >> 7) & not_a_file) |
           ((source_bitset >> 9) & not_h_file);
  }
}

u64 pawn_forward_moves(u64 source_bitset, i32 side) {
  if (side == kWhite) {
    return source_bitset << 8;
  } else {
    return source_bitset >> 8;
  }
}

u64 king_moves(u32 source_idx) { return BITBOARD_KING_ATTACKS[source_idx]; }

u64 knight_moves(u32 source_idx) { return BITBOARD_KNIGHT_ATTACKS[source_idx]; }

u64 bishop_moves(u32 source_idx, u64 occupancy_mask) {
  static bitscan_function bitscan_fns[4] = {bitscan_forward, bitscan_forward,
                                            bitscan_reverse, bitscan_reverse};
  u64 result = 0;
  for (i32 direction = 0; direction < 4; direction++) {
    const u64 ray = BITBOARD_BISHOP_RAYS[direction][source_idx];
    const u64 overlaps = ray & occupancy_mask;
    result |= ray;
    if (overlaps) {
      result &=
          ~(BITBOARD_BISHOP_RAYS[direction][bitscan_fns[direction](overlaps)]);
    }
  }
  return result;
}

u64 rook_moves(u32 source_idx, u64 occupancy_mask) {
  static bitscan_function bitscan_fns[4] = {bitscan_forward, bitscan_forward,
                                            bitscan_reverse, bitscan_reverse};
  u64 result = 0;
  for (i32 direction = 0; direction < 4; direction++) {
    const u64 ray = BITBOARD_ROOK_RAYS[direction][source_idx];
    const u64 overlaps = ray & occupancy_mask;
    result |= ray;
    if (overlaps) {
      result &=
          ~(BITBOARD_ROOK_RAYS[direction][bitscan_fns[direction](overlaps)]);
    }
  }
  return result;
}
