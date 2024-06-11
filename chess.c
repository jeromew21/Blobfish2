#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "bitboard_constants.h"

const char* move_to_string(Move mv, char* buf) {
    // returns string of length 5, i.e. e2-e4
    static char row_names[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    static char col_names[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    u32 src = move_get_src_u32(mv);
    u32 dest = move_get_dest_u32(mv);
    sprintf(buf, "%c%c-%c%c", col_names[src%8], row_names[src/8], col_names[dest%8], row_names[dest/8]);
}

void dump_board(Board *board)
{
    printf("=== board dump ===\n");
    // dump_u64(board->_bitboard[kWhite] | board->_bitboard[kBlack]);
    for (int row = 7; row >= 0; row--)
    {
        printf("|");
        for (int col = 0; col < 8; col++)
        {
            u64 elem = ((u64)1 << (row * 8 + col));
            int y = 0;
            for (int j = 2; j < 8; j++)
            {
                if (board->_bitboard[j] & elem)
                {
                    if (board->_bitboard[kWhite] & elem)
                    {
                        switch (j)
                        {
                        case kPawn:
                            printf("P");
                            break;
                        case kKnight:
                            printf("N");
                            break;
                        default:
                            printf("X");
                            break;
                        }
                    }
                    else
                    {
                        switch (j)
                        {
                        case kPawn:
                            printf("p");
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
            if (y == 0)
            {
                printf(" ");
            }
            printf("|");
        }
        printf("\n");
    }
}

void dump_u64(u64 bitset)
{
    printf("=== u64 dump ===\n");
    for (int row = 7; row >= 0; row--)
    {
        for (int col = 0; col < 8; col++)
        {
            int i = row * 8 + col;
            if ((bitset >> i) & 1)
            {
                printf("1 ");
            }
            else
            {
                printf("0 ");
            }
        }
        printf("\n");
    }
}

Board *default_starting_board()
{
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

Board *board_from_goofy_string(const char *goofy_string)
{
    Board *board = calloc(1, sizeof(Board));
    char clean_goofy_string[64];
    {
        int i = 0;
        for (int k = 0; k < 72; k++)
        {
            char c = goofy_string[k];
            if (c == '\n')
                continue;
            int row = i / 8;
            int col = i % 8;
            clean_goofy_string[(7 - row) * 8 + col] = c;
            i++;
        }
    }
    for (int i = 0; i < 64; i++)
    {
        u64 piece = (u64)1 << i;
        char c = clean_goofy_string[i];
        switch (c)
        {
        case 'N':
            board->_bitboard[kWhite] |= piece;
            board->_bitboard[kKnight] |= piece;
            break;
        case 'P':
            board->_bitboard[kWhite] |= piece;
            board->_bitboard[kPawn] |= piece;
            break;
        case 'p':
            board->_bitboard[kBlack] |= piece;
            board->_bitboard[kPawn] |= piece;
            break;
        }
    }
    return board;
}

int main()
{
    printf("hello world\n");
    Board *board = board_from_goofy_string("........\n........\n........\n..N....N\np.......\nPPPPPPPP\n........\n........");
    dump_board(board);
    MoveList moves = generate_all_pseudo_legal_moves(board);
    printf("Moves count: %i\n", moves.count);
    char buf[5];
    for (int i = 0; i < moves.count; i++) {
        Move mv = move_list_get(&moves, i);
        move_to_string(mv, buf);
        printf("%.5s\n", buf);
    }
    free(board);
}