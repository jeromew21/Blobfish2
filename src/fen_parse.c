#include <stdlib.h>
#include <string.h>

#include "chess.h"

void piece_placement(Board *board, const char *fen, int *i);

void side_to_move(Board *board, const char *fen, int *i);

void castling(Board *board, const char *fen, int *i);

void en_passant(Board *board, const char *fen, int *i);

void halfmove_clock(Board *board, const char *fen, int *i);

void fullmove_counter(Board *board, const char *fen, int *i);

/**
 * https://www.chessprogramming.org/Forsyth-Edwards_Notation
 * TODO: upgrade to Shredder-FEN for 960 support
 * TODO: catch parse errors
 * TODO: the other direction?
 */
Board *board_from_fen(const char *fen) {
    Board *board = calloc(1, sizeof(Board));
    int i = 0;
    piece_placement(board, fen, &i);
    side_to_move(board, fen, &i);
    castling(board, fen, &i);
    en_passant(board, fen, &i);
    halfmove_clock(board, fen, &i);
    fullmove_counter(board, fen, &i);
    return board;
}

void piece_placement(Board *board, const char *fen, int *i) {
    int row = 7;
    int col = 0;
    while (row >= 0) {
        const char c = fen[*i];
        col = col % 8;
        const u64 piece_bitset = (u64) 1 << (row * 8 + col);
        switch (c) {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8': {
                const int count = atoi(&c);
                col += count;
                break;
            }
            case '/':
            case ' ':
                row--;
                break;
            case 'P':
                board->_bitboard[kWhite] |= piece_bitset;
                board->_bitboard[kPawn] |= piece_bitset;
                col++;
                break;
            case 'N':
                board->_bitboard[kWhite] |= piece_bitset;
                board->_bitboard[kKnight] |= piece_bitset;
                col++;
                break;
            case 'K':
                board->_bitboard[kWhite] |= piece_bitset;
                board->_bitboard[kKing] |= piece_bitset;
                col++;
                break;
            case 'Q':
                board->_bitboard[kWhite] |= piece_bitset;
                board->_bitboard[kQueen] |= piece_bitset;
                col++;
                break;
            case 'R':
                board->_bitboard[kWhite] |= piece_bitset;
                board->_bitboard[kRook] |= piece_bitset;
                col++;
                break;
            case 'B':
                board->_bitboard[kWhite] |= piece_bitset;
                board->_bitboard[kBishop] |= piece_bitset;
                col++;
                break;
            case 'p':
                board->_bitboard[kBlack] |= piece_bitset;
                board->_bitboard[kPawn] |= piece_bitset;
                col++;
                break;
            case 'n':
                board->_bitboard[kBlack] |= piece_bitset;
                board->_bitboard[kKnight] |= piece_bitset;
                col++;
                break;
            case 'k':
                board->_bitboard[kBlack] |= piece_bitset;
                board->_bitboard[kKing] |= piece_bitset;
                col++;
                break;
            case 'q':
                board->_bitboard[kBlack] |= piece_bitset;
                board->_bitboard[kQueen] |= piece_bitset;
                col++;
                break;
            case 'r':
                board->_bitboard[kBlack] |= piece_bitset;
                board->_bitboard[kRook] |= piece_bitset;
                col++;
                break;
            case 'b':
                board->_bitboard[kBlack] |= piece_bitset;
                board->_bitboard[kBishop] |= piece_bitset;
                col++;
                break;
            default:
                break;
        }
        (*i)++;
    }
}

void side_to_move(Board *board, const char *fen, int *i) {
    while (1) {
        const char c = fen[*i];
        if (c == 'w') {
            board->_turn = kWhite;
        } else if (c == 'b') {
            board->_turn = kBlack;
        } else if (c == ' ') {
            (*i)++;
            break;
        }
        (*i)++;
    }
}

void castling(Board *board, const char *fen, int *i) {
    BoardMetadata *md = board_metadata_peek(board);
    u32 rights = 0xf;
    while (1) {
        const char c = fen[*i];
        if (c == 'k') {
            rights ^= kBlackKingSideFlag;
        } else if (c == 'q') {
            rights ^= kBlackQueenSideFlag;
        } else if (c == 'K') {
            rights ^= kWhiteKingSideFlag;
        } else if (c == 'Q') {
            rights ^= kWhiteQueenSideFlag;
        } else if (c == ' ') {
            (*i)++;
            break;
        }
        (*i)++;
    }
    board_metadata_set_castling_rights(md, rights);
}

void en_passant(Board *board, const char *fen, int *i) {
    BoardMetadata *md = board_metadata_peek(board);
    char file_letter;
    char rank_number;
    while (1) {
        const char c = fen[*i];
        if (c >= 97 && c <= 104) {
            file_letter = c;
        } else if (c >= 49 && c <= 56) {
            rank_number = c;
        } else if (c == ' ') {
            (*i++);
            break;
        }
        (*i)++;
    }
    int col = file_letter - 97;
    int row = rank_number - 49;
    board_metadata_set_en_passant_square(md, row*8 + col);
}

// TODO
void halfmove_clock(Board *board, const char *fen, int *i) {}

// TODO
void fullmove_counter(Board *board, const char *fen, int *i) {}
