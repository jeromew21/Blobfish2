#include <assert.h>
#include "chess.h"

BoardMetadata *board_metadata_peek(Board* board) {
    if (board->_ply == 0) return &board->_initial_state;
    return &board->_state_stack[board->_ply-1];
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

/**
 * We iterate through the bitboards to find "piece at square".
 * If this ends up being slow for some reason, we can do an old fashioned
 * 2d array of pieces. I'm not convinced this will be the case: we must, must, measure.
 */
void make_move(Board* board, Move mv) {
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
    { // Castling stuff
        BoardMetadata *prev_md = board_metadata_peek(board);
        u32 castling_rights = board_metadata_get_castling_rights(prev_md);
        const u32 king_side_flag = board->_turn == kWhite ? kWhiteKingSideFlag : kBlackKingSideFlag;
        const u32 queen_side_flag = board->_turn == kWhite ? kWhiteQueenSideFlag : kBlackQueenSideFlag;
        if ((board->_bitboard[kKing] & src) ||
            (move_metadata == kKingSideCastleMove || move_metadata == kQueenSideCastleMove)) {
            // Castling or moving king
            castling_rights |= king_side_flag;
            castling_rights |= queen_side_flag;
        }
        if (board->_bitboard[kRook] & src) {
            // Moving rook
            const u32 src_idx = move_get_src_u32(mv);
            const u32 king_idx = bitscan_forward(board->_bitboard[kKing] & board->_bitboard[board->_turn]);
            if (src_idx < king_idx) {
                castling_rights |= queen_side_flag;
            } else {
                castling_rights |= king_side_flag;
            }
        }
        if (board->_bitboard[kRook] & dest) {
            // Capturing a rook
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
    }
    if (dest & board->_bitboard[!board->_turn]) {
        // Find and remove captured piece
        for (u32 i = 2; i < 8; i++) {
            if (board->_bitboard[!board->_turn] & board->_bitboard[i] & dest) {
                board->_bitboard[!board->_turn] ^= dest;
                board->_bitboard[i] ^= dest;
                board_metadata_set_captured_piece(md, i);
                assert(i == board_metadata_get_captured_piece(md));
                break;
            }
        }
    } else {
        // No captured piece
        board_metadata_set_captured_piece(md, 0);
        assert(0 == board_metadata_get_captured_piece(md));
    }
    if (move_metadata == kKingSideCastleMove || move_metadata == kQueenSideCastleMove) {

    } else {
        // Regular move
        for (int i = 2; i < 8; i++) {
            if (board->_bitboard[board->_turn] & board->_bitboard[i] & src) {
                board->_bitboard[board->_turn] ^= src;
                board->_bitboard[board->_turn] ^= dest;
                board->_bitboard[i] ^= src;
                board->_bitboard[i] ^= dest;
                break;
            }
        }
    }
    md->_last_move = mv;
    board->_turn = !board->_turn;
    board->_ply++;
}

void unmake(Board* board) {
    assert (board->_ply > 0);
    BoardMetadata *md = board_metadata_peek(board);
    const Move mv = md->_last_move;
    const u64 src = move_get_src(mv);
    const u64 dest = move_get_dest(mv);
    const u32 move_metadata = move_get_metadata(mv);
    for (int i = 2; i < 8; i++) {
        if (board->_bitboard[!board->_turn] & board->_bitboard[i] & dest) {
            board->_bitboard[!board->_turn] ^= src;
            board->_bitboard[!board->_turn] ^= dest;
            board->_bitboard[i] ^= src;
            board->_bitboard[i] ^= dest;
            u32 captured_piece = board_metadata_get_captured_piece(md);
            if (captured_piece > 0) {
                board->_bitboard[captured_piece] |= dest;
                board->_bitboard[board->_turn] |= board->_bitboard[captured_piece];
            }
            break;
        }
    }
    board->_turn = !board->_turn;
    board->_ply--;
}
