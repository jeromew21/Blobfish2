#include <stdbool.h>
#include <stdlib.h>
#include "chess.h"

void copy_bitboard(u64* dest, u64*copy) {
    for (int i = 0; i < 8; i++) {
        dest[i] = copy[i];
    }
}

bool comp_bitboards(u64* b1, u64*b2, int *out_which) {
    for (int i = 7; i >= 0; i--) {
        if (b1[i] != b2[i]) {
            *out_which = i;
            return false;
        }
    }
    return true;
}

void perft_test_from_file(const char* filename) {

}

PerftResults perft_helper(Board *board, int depth, int top_depth);

PerftResults perft(Board *board, int depth) {
    return perft_helper(board, depth, depth);
}

PerftResults perft_helper(Board *board, int depth, int top_depth)
{
    PerftResults r;
    if (depth == 0) {
        r.nodes = 1;
        r.castles = 0;
        r.ep = 0;
        r.promotions = 0;
        r.captures = 0;
        return r;
    }
    r.nodes = 0;
    r.castles = 0;
    r.ep = 0;
    r.promotions = 0;
    r.captures = 0;
    MoveList moves = generate_all_legal_moves(board);
    u64 copy_board[8];
    copy_bitboard(copy_board, board->_bitboard);
    for (int i = 0; i < moves.count; i++) {
        Move mv = move_list_get(&moves, i);

        u32 move_md = move_get_metadata(mv);
        if (move_md & kCaptureMove) {
            r.captures++;
        }
        if (move_md == kEnPassantMove) {
            r.ep++;
        }
        if (move_md == kQueenSideCastleMove || move_md == kKingSideCastleMove) {
            r.castles++;
        }
        if (move_md & 0b1000) {
            r.promotions++;
        }
        u64 dest = move_get_dest(mv);
        if (dest & board->_bitboard[kKing]) {
            char buf[69];
            dump_board(board);
            move_to_string(mv, buf);
            printf("Move: %s\n", buf);
            exit(69);
        }
        make_move(board, mv);
        PerftResults sub_results = perft_helper(board, depth - 1, top_depth);
//        if (depth == top_depth) {
//            char buf[69];
//            move_to_string(mv, buf);
//            printf("%s: %llu\n", buf, sub_results.nodes);
//        }
        r.nodes += sub_results.nodes;
        r.captures += sub_results.captures;
        r.ep += sub_results.ep;
        r.castles += sub_results.castles;
        r.promotions += sub_results.promotions;
        unmake(board);
        int which;
        if (!comp_bitboards(copy_board, board->_bitboard, &which)) {
            char buf[69];
            printf("EPIC FAIL in UNMAKE!! Iteration %i, Depth %i\n", i, depth);
            printf("which bitboard: %i\n", which);
            move_to_string(mv, buf);
            printf("Move: %s\n", buf);
            printf("Move md: %x\n", move_get_metadata(mv));
            dump_u64(copy_board[which]);
            dump_u64(board->_bitboard[which]);
            exit(69);
        }
    }
    return r;
}