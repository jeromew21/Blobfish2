#include "bitboard_constants.h"
#include "chess.h"
#include <assert.h>
#include <stdio.h>

/**
 * Update hash based on piece movement. Don't update any other hash components.
 */
void hash_update_pieces(u64 *bitboards, i32 turn, Move mv, u64 *hash) {
    const u64 src = move_get_src(mv);
    const u64 dest = move_get_dest(mv);
    const u32 src_idx = move_get_src_u32(mv);
    const u32 dest_idx = move_get_dest_u32(mv);
    const u32 move_metadata = move_get_metadata(mv);
    if ((move_metadata & kCaptureMove) && (move_metadata != kEnPassantMove)) {
        for (u32 i = 2; i < 8; i++) {
            if (bitboards[!turn] & bitboards[i] & dest) {
                (*hash) ^= zobrist_key(i, dest_idx, !turn);
                break;
            }
        }
    }
    if (move_metadata == kQuietMove || move_metadata == kCaptureMove ||
        move_metadata == kDoublePawnMove) {
        for (i32 i = 2; i < 8; i++) {
            if (bitboards[turn] & bitboards[i] & src) {
                (*hash) ^= zobrist_key(i, src_idx, turn);
                (*hash) ^= zobrist_key(i, dest_idx, turn);
                break;
            }
        }
    } else if (move_metadata == kKingSideCastleMove ||
               move_metadata == kQueenSideCastleMove) {
        const u64 rank1 = 0x00000000000000FF;
        const u64 rank8 = 0xFF00000000000000;
        const u64 rank = turn == kWhite ? rank1 : rank8;
        u32 rook_src_idx;
        u32 rook_dest_idx;
        if (move_metadata == kKingSideCastleMove) {
            rook_src_idx = bitscan_reverse(bitboards[kRook] & bitboards[turn] & rank);
            rook_dest_idx = dest_idx - 1;
        } else {
            rook_src_idx = bitscan_forward(bitboards[kRook] & bitboards[turn] & rank);
            rook_dest_idx = dest_idx + 1;
        }
        (*hash) ^= zobrist_key(kKing, src_idx, turn);
        (*hash) ^= zobrist_key(kKing, dest_idx, turn);
        (*hash) ^= zobrist_key(kRook, rook_src_idx, turn);
        (*hash) ^= zobrist_key(kRook, rook_dest_idx, turn);
    } else if (move_metadata & PROMOTION_BIT_FLAG) {
        (*hash) ^= zobrist_key(kPawn, src_idx, turn);
        if (move_metadata == kQueenPromotionMove ||
            move_metadata == kQueenCapturePromotionMove) {
            (*hash) ^= zobrist_key(kQueen, dest_idx, turn);
        } else if (move_metadata == kBishopPromotionMove ||
                   move_metadata == kBishopCapturePromotionMove) {
            (*hash) ^= zobrist_key(kBishop, dest_idx, turn);
        } else if (move_metadata == kKnightPromotionMove ||
                   move_metadata == kKnightCapturePromotionMove) {
            (*hash) ^= zobrist_key(kKnight, dest_idx, turn);
        } else if (move_metadata == kRookPromotionMove ||
                   move_metadata == kRookCapturePromotionMove) {
            (*hash) ^= zobrist_key(kRook, dest_idx, turn);
        }
    } else if (move_metadata == kEnPassantMove) {
        i32 offset = turn == kWhite ? -8 : 8;
        i32 ep_removal_square = ((i32) dest_idx) + offset;
        (*hash) ^= zobrist_key(kPawn, ep_removal_square, !turn);
        (*hash) ^= zobrist_key(kPawn, src_idx, turn);
        (*hash) ^= zobrist_key(kPawn, dest_idx, turn);
    }
}

void bitboards_update(u64 *bitboards, i32 turn, Move mv) {
    const u64 src = move_get_src(mv);
    const u64 dest = move_get_dest(mv);
    const u32 move_metadata = move_get_metadata(mv);
    if ((move_metadata & kCaptureMove) &&
        (move_metadata != kEnPassantMove)) { // EP handled seperately???
        for (int i = 2; i < 8; i++) {
            if (bitboards[!turn] & bitboards[i] & dest) {
                bitboards[!turn] &= ~dest;
                bitboards[i] &= ~dest;
                break;
            }
        }
    }
    if (move_metadata == kQuietMove || move_metadata == kCaptureMove ||
        move_metadata == kDoublePawnMove) {
        for (int i = 2; i < 8; i++) {
            if (bitboards[turn] & bitboards[i] & src) {
                bitboards[turn] &= ~src;
                bitboards[turn] |= dest;
                bitboards[i] &= ~src;
                bitboards[i] |= dest;
                break;
            }
        }
    } else if (move_metadata == kKingSideCastleMove ||
               move_metadata == kQueenSideCastleMove) {
        const u64 rank1 = 0x00000000000000FF;
        const u64 rank8 = 0xFF00000000000000;
        const u64 rank = turn == kWhite ? rank1 : rank8;
        u64 rook_src;
        u64 rook_dest;
        if (move_metadata == kKingSideCastleMove) {
            rook_src =
                    (u64) 1 << bitscan_reverse(bitboards[kRook] & bitboards[turn] & rank);
            rook_dest = dest >> 1;
        } else {
            rook_src =
                    (u64) 1 << bitscan_forward(bitboards[kRook] & bitboards[turn] & rank);
            rook_dest = dest << 1;
        }
        bitboards[turn] &= ~(src | rook_src);
        bitboards[turn] |= dest | rook_dest;
        bitboards[kKing] &= ~src;
        bitboards[kKing] |= dest;
        bitboards[kRook] &= ~rook_src;
        bitboards[kRook] |= rook_dest;
    } else if (move_metadata & PROMOTION_BIT_FLAG) {
        bitboards[turn] &= ~src;
        bitboards[turn] |= dest;
        bitboards[kPawn] &= ~src;
        if (move_metadata == kQueenPromotionMove ||
            move_metadata == kQueenCapturePromotionMove) {
            bitboards[kQueen] |= dest;
        } else if (move_metadata == kBishopPromotionMove ||
                   move_metadata == kBishopCapturePromotionMove) {
            bitboards[kBishop] |= dest;
        } else if (move_metadata == kKnightPromotionMove ||
                   move_metadata == kKnightCapturePromotionMove) {
            bitboards[kKnight] |= dest;
        } else if (move_metadata == kRookPromotionMove ||
                   move_metadata == kRookCapturePromotionMove) {
            bitboards[kRook] |= dest;
        }
    } else if (move_metadata == kEnPassantMove) {
        i32 offset = turn == kWhite ? -8 : 8;
        i32 ep_removal_square = ((i32) move_get_dest_u32(mv)) + offset;
        u64 ep_removal_bitset = (u64) 1 << ep_removal_square;
        bitboards[!turn] &= ~ep_removal_bitset;
        bitboards[kPawn] &= ~ep_removal_bitset;

        bitboards[turn] &= ~src;
        bitboards[turn] |= dest;
        bitboards[kPawn] &= ~src;
        bitboards[kPawn] |= dest;
    }
}

/**
 * We iterate through the bitboards to find "piece at square".
 * If this ends up being slow for some reason, we can do an old fashioned
 * 2d array of pieces. I'm not convinced this will be the case: we must, must,
 * measure. We could extract out part of this to a helper method 'update
 * bitboards' that we might be able to re-use elsewhere.
 */
void board_make_move(Board *board, Move mv) {
    BoardMetadata *md = &board->_state_stack[board->_ply];
    BoardMetadata *prev_md = board_metadata_peek(board, 0);
    u64 *hash = &md->_hash;
    *hash = prev_md->_hash;
    if (board->_turn == kBlack) {
        board->_fullmove_counter += 1;
    }
    const u64 src = move_get_src(mv);
    const u64 dest = move_get_dest(mv);
    const u32 move_metadata = move_get_metadata(mv);
    // Irreversible moves. Maybe we optimize this by shoving these into other
    // places where similar conditions are checked.
    // or cache conditions as booleans to be reused.
    if (src & board->_bitboard[kPawn]) {
        md->_is_irreversible_move = true;
        md->_halfmove_counter = 0;
    } else if (move_metadata == kKingSideCastleMove ||
               move_metadata == kQueenSideCastleMove) {
        md->_is_irreversible_move = true;
        md->_halfmove_counter = 0;
    } else if (move_metadata & CAPTURE_BIT_FLAG) {
        md->_is_irreversible_move = true;
        md->_halfmove_counter = 0;
    } else {
        md->_is_irreversible_move = false;
        md->_halfmove_counter += 1;
    }
    hash_update_pieces(board->_bitboard, board->_turn, mv, hash);
    {
        const u32 prev_ep_square = board_metadata_get_en_passant_square(prev_md);
        if (prev_ep_square > 0) {
            const i32 col = (i32) prev_ep_square % 8;
            *hash ^= ZOBRIST_KEYS[ZOBRIST_EN_PASSANT + col];
        }
    }

    if (move_metadata == kDoublePawnMove) {
        u32 ep_square;
        if (board->_turn == kWhite) {
            ep_square = bitscan_forward(dest >> 8);
        } else {
            ep_square = bitscan_forward(dest << 8);
        }
        board_metadata_set_en_passant_square(md, ep_square);
        const i32 col = (i32) ep_square % 8;
        *hash ^= ZOBRIST_KEYS[ZOBRIST_EN_PASSANT + col];
    } else {
        board_metadata_set_en_passant_square(md, 0);
    }
    u32 castling_rights = board_metadata_get_castling_rights(prev_md);
    const u32 king_side_flag =
            board->_turn == kWhite ? kWhiteKingSideFlag : kBlackKingSideFlag;
    const u32 queen_side_flag =
            board->_turn == kWhite ? kWhiteQueenSideFlag : kBlackQueenSideFlag;
    if ((board->_bitboard[kKing] & src) ||
        (move_metadata == kKingSideCastleMove ||
         move_metadata == kQueenSideCastleMove)) {
        castling_rights |= king_side_flag;
        castling_rights |= queen_side_flag;
    }
    const u64 rank1 = 0x00000000000000FF;
    const u64 rank8 = 0xFF00000000000000;
    const u64 friendly_back_rank = board->_turn == kWhite ? rank1 : rank8;
    const u64 enemy_back_rank = board->_turn == kBlack ? rank1 : rank8;
    if (board->_bitboard[kRook] & src & board->_rook_start_positions &
        friendly_back_rank) {
        const u32 src_idx = move_get_src_u32(mv);
        const u32 king_idx = bitscan_forward(board->_bitboard[kKing] &
                                             board->_bitboard[board->_turn]);
        if (src_idx < king_idx) {
            castling_rights |= queen_side_flag;
        } else {
            castling_rights |= king_side_flag;
        }
    }
    if (board->_bitboard[kRook] & dest & board->_rook_start_positions &
        enemy_back_rank) {
        const u32 enemy_king_side_flag =
                board->_turn == kBlack ? kWhiteKingSideFlag : kBlackKingSideFlag;
        const u32 enemy_queen_side_flag =
                board->_turn == kBlack ? kWhiteQueenSideFlag : kBlackQueenSideFlag;
        const u32 dest_idx = move_get_dest_u32(mv);
        const u32 king_idx = bitscan_forward(board->_bitboard[kKing] &
                                             board->_bitboard[!board->_turn]);
        if (dest_idx < king_idx) {
            castling_rights |= enemy_queen_side_flag;
        } else {
            castling_rights |= enemy_king_side_flag;
        }
    }
    board_metadata_set_castling_rights(md, castling_rights);
    {
        const u32 castling_rights_diff =
                castling_rights ^ board_metadata_get_castling_rights(prev_md);
        for (i32 i = 0; i < 4; i++) {
            const u32 right = ((u32) 1 << i) & castling_rights_diff;
            if (right) {
                (*hash) ^= ZOBRIST_KEYS[ZOBRIST_CASTLING + i];
            }
        }
    }
    {
        u32 captured_piece = 0;
        if (move_metadata == kEnPassantMove) {
            captured_piece = kPawn;
        } else if (move_metadata & CAPTURE_BIT_FLAG) {
            for (int i = 2; i < 8; i++) {
                if (board->_bitboard[!board->_turn] & board->_bitboard[i] & dest) {
                    captured_piece = i;
                    break;
                }
            }
        }
        board_metadata_set_captured_piece(md, captured_piece);
    }
    (*hash) ^=
            ZOBRIST_KEYS[ZOBRIST_BLACK_TO_MOVE]; // alternating in-out each half-move
    bitboards_update(board->_bitboard, board->_turn, mv);
    md->_last_move = mv;
    board->_turn = !board->_turn;
    board->_ply++;
    { // REPITITIONs
        md->_is_repetition = prev_md->_is_repetition;
        if (md->_is_repetition) {
            return;
        }
        const u64 current_hash = md->_hash;
        if (md->_is_irreversible_move) {
            return;
        }
        if (board->_ply >= 1 && board->_state_stack[board->_ply - 1]._is_irreversible_move) {
            return;
        }
        for (int i = (int) board->_ply - 3; i >= 0; i -= 2) {
            BoardMetadata *comp_md = &board->_state_stack[i];
            if (comp_md->_is_irreversible_move) {
                return;
            }
            if (i >= 1 && board->_state_stack[i - 1]._is_irreversible_move) {
                return;
            }
            const u64 comp_hash = comp_md->_hash;
            if (comp_hash == current_hash) {
                md->_is_repetition = true;
                return;
            }
        }
    }
}

void board_unmake(Board *board) {
    // TODO: some more optional (?) assertions for unmake
    assert(board->_ply > 0);
    BoardMetadata *md = board_metadata_peek(board, 0);
    if (board->_turn == kBlack) {
        board->_fullmove_counter -= 1;
    }
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
    } else if (move_metadata & PROMOTION_BIT_FLAG) { // promotion tag
        board->_bitboard[!board->_turn] |= src;
        board->_bitboard[kPawn] |= src; // add pawn back
        if (move_metadata == kQueenPromotionMove ||
            move_metadata == kQueenCapturePromotionMove) {
            board->_bitboard[!board->_turn] &= ~dest; // take away the new thing
            board->_bitboard[kQueen] &= ~dest;
        } else if (move_metadata == kBishopPromotionMove ||
                   move_metadata == kBishopCapturePromotionMove) {
            board->_bitboard[!board->_turn] &= ~dest; // take away the new thing
            board->_bitboard[kBishop] &= ~dest;
        } else if (move_metadata == kKnightPromotionMove ||
                   move_metadata == kKnightCapturePromotionMove) {
            board->_bitboard[!board->_turn] &= ~dest; // take away the new thing
            board->_bitboard[kKnight] &= ~dest;
        } else if (move_metadata == kRookPromotionMove ||
                   move_metadata == kRookCapturePromotionMove) {
            board->_bitboard[!board->_turn] &= ~dest; // take away the new thing
            board->_bitboard[kRook] &= ~dest;
        }
        if (move_metadata & kCaptureMove) {
            u32 captured_piece = board_metadata_get_captured_piece(md);
            board->_bitboard[captured_piece] |= dest;
            board->_bitboard[board->_turn] |= dest;
        }
    } else if (move_metadata == kKingSideCastleMove ||
               move_metadata == kQueenSideCastleMove) {
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
                if (move_metadata & CAPTURE_BIT_FLAG) {
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
