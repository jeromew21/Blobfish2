#include <assert.h>
#include <stdlib.h>
#include "chess.h"
#include "bitboard_constants.h"
#include "move_generation.h"

/*Intrinsics not working on Mac
    TODO: try using on Linux
*/
/*#include <immintrin.h>*/

typedef u32 (*bitscan_function)(u64);

u64 bishop_moves(u32 source_idx, u64 occupancy_mask);

u64 rook_moves(u32 source_idx, u64 occupancy_mask);

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
        u64 jumps = BITBOARD_KNIGHT_ATTACKS[idx] & ~friendly_mask;
        while (jumps) {
            const u32 dest_idx = bitscan_forward(jumps);
            const u64 dest_bit = (u64) 1 << dest_idx;
            jumps ^= dest_bit;
            const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
            const Move mv = move_create(idx, dest_idx, md);
            move_list_push(&move_list, mv);
        }
        knights ^= (u64) 1 << idx;
    }
    // Castling
    // TODO: generalized, 960 castling the rule is there's a king target square for all positions
    const u64 king = friendly_mask & board->_bitboard[kKing];
    if (king) {
        BoardMetadata *md = board_metadata_peek(board);
        const u32 castling_rights = board_metadata_get_castling_rights(md);
        if ((board->_turn == kWhite && !(castling_rights & kWhiteKingSideFlag)) ||
            (board->_turn == kBlack && !(castling_rights & kBlackKingSideFlag))) {
            const u32 king_dest = board->_turn == kWhite ? 6 : 62;
            // 960: need to fix this
            const u64 king_squares = king | (king << 1) | (king << 2); // these must all not be attacked
            const u64 must_be_empty_squares = king ^ king_squares;
            if (!(must_be_empty_squares & occupancy_mask)) {
                // Should we push the rook instead of the king?
                // Or even encode something entirely different?
                const Move mv = move_create(bitscan_forward(king), king_dest, kKingSideCastleMove);
                move_list_push(&move_list, mv);
            }
        }
        if ((board->_turn == kWhite && !(castling_rights & kWhiteQueenSideFlag)) ||
            (board->_turn == kBlack && !(castling_rights & kBlackQueenSideFlag))) {
            const u32 king_dest = board->_turn == kWhite ? 2 : 58;
            // 960: need to fix this
            const u64 king_squares = king | (king >> 1) | (king >> 2);
            const u64 must_be_empty_squares = king ^ (king_squares | (king >> 3));
            if (!(must_be_empty_squares & occupancy_mask)) {
                const Move mv = move_create(bitscan_forward(king), king_dest, kKingSideCastleMove);
                move_list_push(&move_list, mv);
            }
        }
    }
    // King
    // Same note as for Knights w.r.t. optimization
    if (king) { // technically this check isn't needed for legal positions
        const u32 king_idx = bitscan_forward(king);
        u64 king_moves = BITBOARD_KING_ATTACKS[king_idx] & ~friendly_mask;
        while (king_moves) {
            const u32 dest_idx = bitscan_forward(king_moves);
            const u64 dest_bit = (u64) 1 << dest_idx;
            king_moves ^= dest_bit;
            const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
            const Move mv = move_create(king_idx, dest_idx, md);
            move_list_push(&move_list, mv);
        }
    }
    // Pawns
    // TODO: en passant
    // TODO: promotions
    const u64 pawns = friendly_mask & board->_bitboard[kPawn];
    if (pawns) {
        const u64 empty_mask = ~occupancy_mask;
        const u64 not_a_file = ~0x0101010101010101;
        const u64 not_h_file = ~0x8080808080808080;
        const u64 en_passant_square = (u64) 1 << board_metadata_get_en_passant_square(board_metadata_peek(board));
        u64 pawn_east_attacks;
        u64 pawn_west_attacks;
        u64 pawn_jumps_single;
        u64 pawn_double_push_destinations; // locations of pawns that can double push
        u32 east_offset;
        u32 west_offset;
        u32 pawn_jump_offset;
        if (board->_turn == kWhite) {
            const u64 fourth_rank = 0x00000000FF000000;
            pawn_east_attacks = ((pawns << 9) & not_a_file) & (en_passant_square | enemy_mask);
            pawn_west_attacks = ((pawns << 7) & not_h_file) & (en_passant_square | enemy_mask);
            pawn_jumps_single = (pawns << 8) & empty_mask;
            east_offset = -9;
            west_offset = -7;
            pawn_jump_offset = -8;
            pawn_double_push_destinations = (pawn_jumps_single << 8) & empty_mask & fourth_rank;
        } else {
            const u64 fifth_rank = 0x000000FF00000000;
            pawn_east_attacks = ((pawns >> 7) & not_a_file) & (en_passant_square | enemy_mask);
            pawn_west_attacks = ((pawns >> 9) & not_h_file) & (en_passant_square | enemy_mask);
            east_offset = 7;
            west_offset = 9;
            pawn_jump_offset = 8;
            pawn_jumps_single = (pawns >> 8) & empty_mask;
            pawn_double_push_destinations = (pawn_jumps_single >> 8) & empty_mask & fifth_rank;
        }
        while (pawn_east_attacks) {
            const u32 dest_idx = bitscan_forward(pawn_east_attacks);
            const u32 src_idx = dest_idx + east_offset;
            const u64 dest_bit = (u64) 1 << dest_idx;
            pawn_east_attacks ^= dest_bit;
            u32 flags = (dest_bit & en_passant_square) ? kEnPassantMove : kCaptureMove;
            const Move mv = move_create(src_idx, dest_idx, flags);
            move_list_push(&move_list, mv);
        }
        while (pawn_west_attacks) {
            const u32 dest_idx = bitscan_forward(pawn_west_attacks);
            const u32 src_idx = dest_idx + west_offset;
            const u64 dest_bit = (u64) 1 << dest_idx;
            pawn_west_attacks ^= dest_bit;
            u32 flags = (dest_bit & en_passant_square) ? kEnPassantMove : kCaptureMove;
            const Move mv = move_create(src_idx, dest_idx, flags);
            move_list_push(&move_list, mv);
        }
        while (pawn_jumps_single) {
            const u32 dest_idx = bitscan_forward(pawn_jumps_single);
            const u32 src_idx = dest_idx + pawn_jump_offset;
            pawn_jumps_single ^= (u64) 1 << dest_idx;
            const Move mv = move_create(src_idx, dest_idx, 0);
            move_list_push(&move_list, mv);
        }
        while (pawn_double_push_destinations) {
            const u32 dest_idx = bitscan_forward(pawn_double_push_destinations);
            const u32 src_idx = dest_idx + pawn_jump_offset + pawn_jump_offset;
            pawn_double_push_destinations ^= (u64) 1 << dest_idx;
            const Move mv = move_create(src_idx, dest_idx, kDoublePawnMove);
            move_list_push(&move_list, mv);
        }
    }
    u64 bishops = friendly_mask & (board->_bitboard[kBishop] | board->_bitboard[kQueen]);
    while (bishops) {
        const u32 src_idx = bitscan_forward(bishops);
        u64 destinations = bishop_moves(src_idx, occupancy_mask) & ~friendly_mask;
        while (destinations) {
            const u32 dest_idx = bitscan_forward(destinations);
            const u64 dest_bit = (u64) 1 << dest_idx;
            destinations ^= dest_bit;
            const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
            const Move mv = move_create(src_idx, dest_idx, md);
            move_list_push(&move_list, mv);
        }
        bishops ^= (u64) 1 << src_idx;
    }
    u64 rooks = friendly_mask & (board->_bitboard[kRook] | board->_bitboard[kQueen]);
    while (rooks) {
        const u32 src_idx = bitscan_forward(rooks);
        u64 destinations = rook_moves(src_idx, occupancy_mask) & ~friendly_mask;
        while (destinations) {
            const u32 dest_idx = bitscan_forward(destinations);
            const u64 dest_bit = (u64) 1 << dest_idx;
            destinations ^= dest_bit;
            const u32 md = (dest_bit & enemy_mask) ? kCaptureMove : kQuietMove;
            const Move mv = move_create(src_idx, dest_idx, md);
            move_list_push(&move_list, mv);
        }
        rooks ^= (u64) 1 << src_idx;
    }
    return move_list;
}

u64 bishop_moves(u32 source_idx, u64 occupancy_mask) {
    static bitscan_function bitscan_fns[4] = {bitscan_forward, bitscan_forward, bitscan_reverse, bitscan_reverse};
    u64 result = 0;
    for (i32 direction = 0; direction < 4; direction++) {
        const u64 ray = BITBOARD_BISHOP_RAYS[direction][source_idx];
        const u64 overlaps = ray & occupancy_mask;
        result |= ray;
        if (overlaps) {
            result &= ~(BITBOARD_BISHOP_RAYS[direction][bitscan_fns[direction](overlaps)]);
        }
    }
    return result;
}

u64 rook_moves(u32 source_idx, u64 occupancy_mask) {
    static bitscan_function bitscan_fns[4] = {bitscan_forward, bitscan_forward, bitscan_reverse, bitscan_reverse};
    u64 result = 0;
    for (i32 direction = 0; direction < 4; direction++) {
        const u64 ray = BITBOARD_ROOK_RAYS[direction][source_idx];
        const u64 overlaps = ray & occupancy_mask;
        result |= ray;
        if (overlaps) {
            result &= ~(BITBOARD_ROOK_RAYS[direction][bitscan_fns[direction](overlaps)]);
        }
    }
    return result;
}