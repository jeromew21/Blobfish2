#include <assert.h>
#include "chess.h"

/**
 * We iterate through the bitboards to find "piece at square".
 * If this ends up being slow for some reason, we can do an old fashioned
 * 2d array of pieces. I'm not convinced this will be the case: we must, must, measure.
 */
void make_move(Board* board, Move mv) {
    BoardMetadata *md = &board->_state_stack[board->_ply];
    md->_last_move = mv;
    //consult castling rights and EP square
    //md._castling_rights
    //md._en_passant_square
    
    u64 src = move_get_src(mv);
    u64 dest = move_get_dest(mv);
    u32 move_metadata = move_get_metadata(mv);
    if (dest & board->_bitboard[!board->_turn]) {
        for (int i = 2; i < 8; i++) {
            if (board->_bitboard[!board->_turn] & board->_bitboard[i] & dest) {
                board->_bitboard[!board->_turn] ^= dest;
                board->_bitboard[i] ^= dest;
                md->_captured_piece = i;
                break;
            }
        }
    } else {
        md->_captured_piece = -1;
    }
    for (int i = 2; i < 8; i++) {
        if (board->_bitboard[board->_turn] & board->_bitboard[i] & src) {
            board->_bitboard[board->_turn] ^= src;
            board->_bitboard[board->_turn] ^= dest;
            board->_bitboard[i] ^= src;
            board->_bitboard[i] ^= dest;
            break;
        }
    }

    board->_turn = !board->_turn;
    board->_ply++;
}

void unmake(Board* board) {
    assert (board->_ply > 0);
    BoardMetadata *md = &board->_state_stack[board->_ply-1];
    Move mv = md->_last_move;

    u64 src = move_get_src(mv);
    u64 dest = move_get_dest(mv);
    u32 move_metadata = move_get_metadata(mv);
    for (int i = 2; i < 8; i++) {
        if (board->_bitboard[!board->_turn] & board->_bitboard[i] & dest) {
            board->_bitboard[!board->_turn] ^= src;
            board->_bitboard[!board->_turn] ^= dest;
            board->_bitboard[i] ^= src;
            board->_bitboard[i] ^= dest;
            if (md->_captured_piece >= 0) {
                board->_bitboard[md->_captured_piece] |= dest;
                board->_bitboard[board->_turn] |= board->_bitboard[md->_captured_piece];
            }
            break;
        }
    }

    board->_turn = !board->_turn;
    board->_ply--;
}
