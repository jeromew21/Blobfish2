#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "bitboard_constants.h"

void move_to_string(Move mv, char *buf) {
    // returns string of length 5, i.e. e2-e4
    static char row_names[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    static char col_names[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    u32 src = move_get_src_u32(mv);
    u32 dest = move_get_dest_u32(mv);
    //printf("%c%c-%c%c", col_names[src%8], row_names[src/8], col_names[dest%8], row_names[dest/8]);
    sprintf(buf, "%c%c-%c%c", col_names[src % 8], row_names[src / 8], col_names[dest % 8], row_names[dest / 8]);
}

void dump_board(Board *board) {
    printf("======== board dump ========\n");
    BoardMetadata *md = board_metadata_peek(board);
    printf("Ply: %i\n", board->_ply);
    printf("Turn: %s\n", board->_turn == kWhite ? "White" : "Black");
    {
        u32 rights = board_metadata_get_castling_rights(md) & 0xf;
        printf("Castling rights: 0x%x\n", rights & 0xf);
        printf("\tWhite king-side: %s\n", rights & kWhiteKingSideFlag ? "no" : "yes");
        printf("\tWhite queen-side: %s\n", rights & kWhiteQueenSideFlag ? "no" : "yes");
        printf("\tBlack king-side: %s\n", rights & kBlackKingSideFlag ? "no" : "yes");
        printf("\tBlack queen-side: %s\n", rights & kBlackQueenSideFlag ? "no" : "yes");
    }
    printf("En Passant Square: %i\n", board_metadata_get_en_passant_square(md) & 0xff);

    // dump_u64(board->_bitboard[kWhite] | board->_bitboard[kBlack]);
    for (int row = 7; row >= 0; row--) {
        printf("|");
        for (int col = 0; col < 8; col++) {
            u64 elem = ((u64) 1 << (row * 8 + col));
            int y = 0;
            for (int j = 2; j < 8; j++) {
                if (board->_bitboard[j] & elem) {
                    if (board->_bitboard[kWhite] & elem) {
                        switch (j) {
                            case kPawn:
                                printf("P");
                                break;
                            case kKnight:
                                printf("N");
                                break;
                            case kRook:
                                printf("R");
                                break;
                            case kBishop:
                                printf("B");
                                break;
                            case kQueen:
                                printf("Q");
                                break;
                            case kKing:
                                printf("K");
                                break;
                            default:
                                printf("X");
                                break;
                        }
                    } else {
                        switch (j) {
                            case kPawn:
                                printf("p");
                                break;
                            case kKnight:
                                printf("n");
                                break;
                            case kRook:
                                printf("r");
                                break;
                            case kBishop:
                                printf("b");
                                break;
                            case kQueen:
                                printf("q");
                                break;
                            case kKing:
                                printf("k");
                                break;
                            default:
                                printf("x");
                                break;
                        }
                    }
                    y = 1;
                    break;
                }
            }
            if (y == 0) {
                printf(" ");
            }
            printf("|");
        }
        printf("\n");
    }
    printf("========    end     ========\n");
}

void dump_u64(u64 bitset) {
    printf("=== u64 dump ===\n");
    for (int row = 7; row >= 0; row--) {
        for (int col = 0; col < 8; col++) {
            int i = row * 8 + col;
            if ((bitset >> i) & 1) {
                printf("1 ");
            } else {
                printf("0 ");
            }
        }
        printf("\n");
    }
}

Board *board_default_starting_position() {
    Board *board = calloc(1, sizeof(Board));
    board->_bitboard[kWhite] = 0x000000000000FFFF;
    board->_bitboard[kBlack] = 0xFFFF000000000000;
    board->_bitboard[kPawn] = 0x00FF00000000FF00;
    board->_bitboard[kQueen] = 0x0800000000000008;
    board->_bitboard[kKing] = 0x1000000000000010;
    board->_bitboard[kBishop] = 0x2400000000000024;
    board->_bitboard[kKnight] = 0x4200000000000042;
    board->_bitboard[kRook] = 0x8100000000000081;
    return board;
}

int main() {
    srand(0);
    printf("hello world\n");
    Board *board = board_from_fen("rnbqkbnr/pp1p1pp1/7p/2pPp3/4P3/8/PPP2PPP/RNBQKBNR w KQkq c6 0 4");
    dump_board(board);
    MoveList moves = generate_all_pseudo_legal_moves(board);
    printf("%i moves generated\n", moves.count);
    free(board);
}