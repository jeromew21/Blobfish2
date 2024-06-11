#include <assert.h>
#include "chess.h"

BoardMetadata *board_metadata_peek(Board *board, int i) {
    if (board->_ply == 0) return &board->_initial_state;
    return &board->_state_stack[board->_ply - 1 - i];
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

void bitboards_update(u64 *bitboards, i32 turn, Move mv) {
    const u64 src = move_get_src(mv);
    const u64 dest = move_get_dest(mv);
    const u32 move_metadata = move_get_metadata(mv);
    if (move_metadata == kEnPassantMove) {
        i32 offset = turn == kWhite ? -8 : 8;
        i32 ep_removal_square = ((i32) move_get_dest_u32(mv)) + offset;
        u64 ep_removal_bitset = (u64) 1 << ep_removal_square;
        bitboards[!turn] &= ~ep_removal_bitset;
        bitboards[kPawn] &= ~ep_removal_bitset;
    } else if (move_metadata & kCaptureMove) { // capture bit flag
        for (int i = 2; i < 8; i++) {
            if (bitboards[!turn] & bitboards[i] & dest) {
                bitboards[!turn] &= ~dest;
                bitboards[i] &= ~dest;
                break;
            }
        }
    }
    if (move_metadata == kEnPassantMove) {
        bitboards[turn] &= ~src;
        bitboards[turn] |= dest;
        bitboards[kPawn] &= ~src;
        bitboards[kPawn] |= dest;
    } else if (move_metadata & 0b1000) { // promotion bit flag
        bitboards[turn] &= ~src;
        bitboards[turn] |= dest;
        bitboards[kPawn] &= ~src;
        if (move_metadata == kQueenPromotionMove || move_metadata == kQueenCapturePromotionMove) {
            bitboards[kQueen] |= dest;
        } else if (move_metadata == kBishopPromotionMove || move_metadata == kBishopCapturePromotionMove) {
            bitboards[kBishop] |= dest;
        } else if (move_metadata == kKnightPromotionMove || move_metadata == kKnightCapturePromotionMove) {
            bitboards[kKnight] |= dest;
        } else if (move_metadata == kRookPromotionMove || move_metadata == kRookCapturePromotionMove) {
            bitboards[kRook] |= dest;
        }
    } else if (move_metadata == kKingSideCastleMove || move_metadata == kQueenSideCastleMove) {
        u64 rook_src;
        u64 rook_dest;
        if (move_metadata == kKingSideCastleMove) {
            rook_src = (u64) 1 << bitscan_reverse(bitboards[kRook] & bitboards[turn]);
            rook_dest = dest >> 1;
        } else {
            rook_src = (u64) 1 << bitscan_forward(bitboards[kRook] & bitboards[turn]);
            rook_dest = dest << 1;
        }
        bitboards[turn] &= ~(src | rook_src);
        bitboards[turn] |= dest | rook_dest;
        bitboards[kKing] &= ~src;
        bitboards[kKing] |= dest;
        bitboards[kRook] &= ~rook_src;
        bitboards[kRook] |= rook_dest;
    } else {
        for (int i = 2; i < 8; i++) {
            if (bitboards[turn] & bitboards[i] & src) {
                bitboards[turn] &= ~src;
                bitboards[turn] |= dest;
                bitboards[i] &= ~src;
                bitboards[i] |= dest;
                break;
            }
        }
    }
}

/**
 * We iterate through the bitboards to find "piece at square".
 * If this ends up being slow for some reason, we can do an old fashioned
 * 2d array of pieces. I'm not convinced this will be the case: we must, must, measure.
 * We could extract out part of this to a helper method 'update bitboards'
 * that we might be able to re-use elsewhere.
 */
void make_move(Board *board, Move mv) {
    BoardMetadata *md = &board->_state_stack[board->_ply];
    const u64 src = move_get_src(mv);
    const u64 dest = move_get_dest(mv);
    const u32 move_metadata = move_get_metadata(mv);
    if (move_metadata == kDoublePawnMove) {
        u32 ep_square;
        if (board->_turn == kWhite) {
            ep_square = bitscan_forward(dest >> 8);
        } else {
            ep_square = bitscan_forward(dest << 8);
        }
        board_metadata_set_en_passant_square(md, ep_square);
    } else {
        board_metadata_set_en_passant_square(md, 0);
    }
    // Castling stuff
    BoardMetadata *prev_md = board_metadata_peek(board, 0);
    u32 castling_rights = board_metadata_get_castling_rights(prev_md);
    const u32 king_side_flag = board->_turn == kWhite ? kWhiteKingSideFlag : kBlackKingSideFlag;
    const u32 queen_side_flag = board->_turn == kWhite ? kWhiteQueenSideFlag : kBlackQueenSideFlag;
    if ((board->_bitboard[kKing] & src) ||
        (move_metadata == kKingSideCastleMove || move_metadata == kQueenSideCastleMove)) {
        castling_rights |= king_side_flag;
        castling_rights |= queen_side_flag;
    }
    if (board->_bitboard[kRook] & src & board->_rook_start_positions) {
        const u32 src_idx = move_get_src_u32(mv);
        const u32 king_idx = bitscan_forward(board->_bitboard[kKing] & board->_bitboard[board->_turn]);
        if (src_idx < king_idx) {
            castling_rights |= queen_side_flag;
        } else {
            castling_rights |= king_side_flag;
        }
    }
    if (board->_bitboard[kRook] & dest & board->_rook_start_positions) {
        const u32 enemy_king_side_flag = board->_turn == kBlack ? kWhiteKingSideFlag : kBlackKingSideFlag;
        const u32 enemy_queen_side_flag = board->_turn == kBlack ? kWhiteQueenSideFlag : kBlackQueenSideFlag;
        const u32 dest_idx = move_get_dest_u32(mv);
        const u32 king_idx = bitscan_forward(board->_bitboard[kKing] & board->_bitboard[!board->_turn]);
        if (dest_idx < king_idx) {
            castling_rights |= enemy_queen_side_flag;
        } else {
            castling_rights |= enemy_king_side_flag;
        }
    }
    board_metadata_set_castling_rights(md, castling_rights);
    u32 captured_piece = 0;
    if (move_metadata == kEnPassantMove) {
        captured_piece = kPawn;
    } else if (move_metadata & kCaptureMove) {
        for (int i = 2; i < 8; i++) {
            if (board->_bitboard[!board->_turn] & board->_bitboard[i] & dest) {
                captured_piece = i;
                break;
            }
        }
    }
    board_metadata_set_captured_piece(md, captured_piece);
    bitboards_update(board->_bitboard, board->_turn, mv);
    md->_last_move = mv;
    board->_turn = !board->_turn;
    board->_ply++;
}

void unmake(Board *board) {
    // TODO: some more optional (?) checks for unmake
    assert (board->_ply > 0);
    BoardMetadata *md = board_metadata_peek(board, 0);
    const Move mv = md->_last_move;
    const u64 src = move_get_src(mv);
    const u64 dest = move_get_dest(mv);
    const u32 move_metadata = move_get_metadata(mv);
    if (move_metadata == kEnPassantMove) {
        i32 offset = (!board->_turn) == kWhite ? -8 : 8;
        i32 ep_removal_square = ((i32) move_get_dest_u32(mv)) + offset;
        u64 ep_removal_bitset = (u64) 1 << ep_removal_square;
        board->_bitboard[board->_turn] |= ep_removal_bitset;
        board->_bitboard[kPawn] |= ep_removal_bitset;
        board->_bitboard[!board->_turn] |= src;
        board->_bitboard[!board->_turn] &= ~dest;
        board->_bitboard[kPawn] |= src;
        board->_bitboard[kPawn] &= ~dest;
    } else if (move_metadata & 0b1000) { // promotion tag
        board->_bitboard[!board->_turn] |= src;
        board->_bitboard[kPawn] |= src; // add pawn back
        if (move_metadata == kQueenPromotionMove || move_metadata == kQueenCapturePromotionMove) {
            board->_bitboard[!board->_turn] &= ~dest; // take away the new thing
            board->_bitboard[kQueen] &= ~dest;
        } else if (move_metadata == kBishopPromotionMove || move_metadata == kBishopCapturePromotionMove) {
            board->_bitboard[!board->_turn] &= ~dest; // take away the new thing
            board->_bitboard[kBishop] &= ~dest;
        } else if (move_metadata == kKnightPromotionMove || move_metadata == kKnightCapturePromotionMove) {
            board->_bitboard[!board->_turn] &= ~dest; // take away the new thing
            board->_bitboard[kKnight] &= ~dest;
        } else if (move_metadata == kRookPromotionMove || move_metadata == kRookCapturePromotionMove) {
            board->_bitboard[!board->_turn] &= ~dest; // take away the new thing
            board->_bitboard[kRook] &= ~dest;
        }
        if (move_metadata & kCaptureMove) {
            u32 captured_piece = board_metadata_get_captured_piece(md);
            board->_bitboard[captured_piece] |= dest;
            board->_bitboard[board->_turn] |= dest;
        }
    } else if (move_metadata == kKingSideCastleMove || move_metadata == kQueenSideCastleMove) {
        const u64 rank1 = 0x00000000000000FF;
        const u64 rank8 = 0xFF00000000000000;
        const u64 rank = (!board->_turn) == kWhite ? rank1 : rank8;
        u64 rook_src;
        u64 rook_dest;
        if (move_metadata == kKingSideCastleMove) {
            rook_src = (u64) 1 << bitscan_reverse(board->_rook_start_positions & rank);
            rook_dest = dest >> 1;
        } else {
            rook_src = (u64) 1 << bitscan_forward(board->_rook_start_positions & rank);
            rook_dest = dest << 1;
        }
        board->_bitboard[!board->_turn] |= src | rook_src;
        board->_bitboard[!board->_turn] &= ~(dest | rook_dest);
        board->_bitboard[kKing] |= src;
        board->_bitboard[kKing] &= ~dest;
        board->_bitboard[kRook] |= rook_src;
        board->_bitboard[kRook] &= ~rook_dest;
    } else {
        for (int i = 2; i < 8; i++) {
            if (board->_bitboard[!board->_turn] & board->_bitboard[i] & dest) {
                board->_bitboard[!board->_turn] |= src;
                board->_bitboard[!board->_turn] &= ~dest;
                board->_bitboard[i] |= src;
                board->_bitboard[i] &= ~dest;
                if (move_metadata & 0b0100) {
                    u32 captured_piece = board_metadata_get_captured_piece(md);
                    board->_bitboard[captured_piece] |= dest;
                    board->_bitboard[board->_turn] |= dest;
                }
                break;
            }
        }
    }
    board->_turn = !board->_turn;
    board->_ply--;
}
