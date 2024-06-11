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
    char special[256];
    special[0] = '\0';
    u32 md = move_get_metadata(mv);
    if (md == kEnPassantMove) {
        sprintf(special, "%s", " en passant");
    } else if (md == kKingSideCastleMove) {
        sprintf(special, "%s", " O-O");
    } else if (md == kQueenSideCastleMove) {
        sprintf(special, "%s", " O-O-O");
    }
    sprintf(buf, "%c%c-%c%c%s", col_names[src % 8], row_names[src / 8], col_names[dest % 8], row_names[dest / 8], special);
}

int main() {
    srand(0);
    printf("hello world\n");
    Board *board = board_from_fen("rnbqk2r/pp3ppp/2pp1n2/4p3/1b1PP3/2PB1N2/PP3PPP/RNBQK2R w KQkq - 0 6");
    dump_board(board);
    while (1) {
        char line[256];
        MoveList moves = generate_all_legal_moves(board);
        printf("%i moves generated\n", moves.count);
        for (int i = 0; i < moves.count; i++) {
            char buf[256];
            move_to_string(move_list_get(&moves, i), buf);
            printf("%i. %s\n", i, buf);
        }
        if (fgets(line, sizeof(line), stdin) == NULL) {
            exit(1);
        }
        int idx = atoi(line);
        printf("you chose move number %i\n", idx);
        make_move(board, move_list_get(&moves, idx));
        dump_board(board);
        unmake(board);
        dump_board(board);
        break;
    }
    free(board);
}