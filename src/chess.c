#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "bitboard_constants.h"

void board_make_move_from_alg(Board* board) {

}

void move_to_string(Move mv, char *buf) {
    // returns string of length 5, i.e. e2-e4
    static char row_names[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    static char col_names[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    u32 src = move_get_src_u32(mv);
    u32 dest = move_get_dest_u32(mv);
    //printf("%c%c-%c%c", col_names[src%8], row_names[src/8], col_names[dest%8], row_names[dest/8]);
    char special[256];
    special[0] = '\0';
    u32 md = move_get_metadata(mv);
    if (md == kEnPassantMove) {
        sprintf(special, "%s", " en passant");
    } else if (md == kKingSideCastleMove) {
        sprintf(special, "%s", " O-O");
    } else if (md == kQueenSideCastleMove) {
        sprintf(special, "%s", " O-O-O");
    } else if (md & 0b1000) {
        if (md == kQueenPromotionMove) {
            sprintf(special, "%s", " Q");
        } else if (md == kQueenCapturePromotionMove) {
            sprintf(special, "%s", " Q Capture");
        } else {
            sprintf(special, "%s", " under promotion");
        }
    }
    sprintf(buf, "%c%c%c%c%s", col_names[src % 8], row_names[src / 8], col_names[dest % 8], row_names[dest / 8], special);
}

int main() {
    srand(0);
    {
        Board *board = board_from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
        int depth = 4;
        PerftResults pr = perft(board, depth);
        printf("Perft (kiwipete) %i = %llu, %llu, %llu, %llu\n", depth, pr.nodes, pr.captures, pr.ep, pr.castles);
        free(board);
    }
    {
        Board *board = board_default_starting_position();
        int depth = 5;
        PerftResults pr = perft(board, depth);
        printf("Perft (classic) %i = %llu, %llu, %llu, %llu\n", depth, pr.nodes, pr.captures, pr.ep, pr.castles);
        free(board);
    }
    {
        Board *board = board_from_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
        int depth = 4;
        PerftResults pr = perft(board, depth);
        printf("Perft (Position 4) %i = %llu, %llu, %llu, %llu, %llu\n", depth, pr.nodes, pr.captures, pr.ep, pr.castles, pr.promotions);
    }
    {
        Board *board = board_from_fen("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");
        int depth = 1;
        PerftResults pr = perft(board, depth);
        printf("Perft (Position ??) %i = %llu, %llu, %llu, %llu, %llu\n", depth, pr.nodes, pr.captures, pr.ep, pr.castles, pr.promotions);
        free(board);
    }
    {
        Board *board = board_from_fen("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");
        int depth = 4;
        PerftResults pr = perft(board, depth);
        printf("Perft (Promotion Bugcatch) %i = %llu, %llu, %llu, %llu\n", depth, pr.nodes, pr.captures, pr.ep, pr.castles);
        free(board);
    }
    {
        Board *board = board_from_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ");
        int depth = 4;
        PerftResults pr = perft(board, depth);
        printf("Perft (Position 5) %i = %llu, %llu, %llu, %llu\n", depth, pr.nodes, pr.captures, pr.ep, pr.castles);
        free(board);
    }
//    Board *board = board_from_fen("r6r/p1pkqpb1/bn2Pnp1/8/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQ - 0 2");
//    dump_board(board);
//    while (1) {
//        char line[256];
//        MoveList moves = generate_all_legal_moves(board);
//        printf("Found %i moves\n", moves.count);
//        for (int i = 0; i < moves.count; i++) {
//            char buf[256];
//            Move mv = move_list_get(&moves, i);
//            move_to_string(mv, buf);
//            printf("%i %s\n", i, buf);
//        }
//        Move mv = move_list_get(&moves, 11);
//        make_move(board, mv);
//        dump_board(board);
//        break;
//    }
//    free(board);
}