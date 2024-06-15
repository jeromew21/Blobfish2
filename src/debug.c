#include "chess.h"

void dump_bitboard(u64 *bitboards) {
    for (int row = 7; row >= 0; row--) {
        printf("|");
        for (int col = 0; col < 8; col++) {
            u64 elem = ((u64) 1 << (row * 8 + col));
            int y = 0;
            for (int j = 2; j < 8; j++) {
                if (bitboards[j] & elem) {
                    if (bitboards[kWhite] & elem) {
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
}

void board_dump(Board *board) {
    printf("======== board dump ========\n");
    BoardMetadata *md = board_metadata_peek(board, 0);
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
    dump_bitboard(board->_bitboard);
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
