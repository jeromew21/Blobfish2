#include "search.h"
#include "chess.h"
#include <float.h>

Centipawns search_recursive(Board *board, Centipawns alpha, Centipawns beta,
                            i32 depth, _Atomic(bool) *stop, Move *best_move, u64 *nodes_searched);

Centipawns qsearch(Board *board, Centipawns alpha, Centipawns beta,
                   _Atomic(bool) *stop, u64 *nodes_searched);

void search(Board* board, Move* best_move, _Atomic(bool) *stop_thinking, FILE *outfile) {
    // TODO: multi threading
    // TODO: search subset of moves
    // TODO: don't return best move in recursive impl, use root node search
    // TODO: alpha-beta prune at root? why or why not?
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    int ply_depth = 0;
    u64 nodes_searched = 0;
    MoveList moves = generate_all_legal_moves(board);
    (*best_move) = move_list_get(&moves, 0);
    while (1) {
        if (*stop_thinking)
            return;
        Move best_move_found = move_list_get(&moves, 0);
        Move pv_move;
        Centipawns best_score_found = MIN_EVAL;
        Centipawns alpha = MIN_EVAL;
        Centipawns beta = -MIN_EVAL;
        for (int i = 0; i < moves.count; i++) {
            if (*stop_thinking)
                return; // or at least, break out of the while loop
            Move mv = move_list_get(&moves, i);
            board_make_move(board, mv);
            Centipawns score = -search_recursive(board, -beta, -alpha, ply_depth,
                                                 stop_thinking, &pv_move, &nodes_searched);
            if (score > best_score_found) {
                best_score_found = score;
                best_move_found = mv;
            }
            board_unmake(board);
        }
        (*best_move) = best_move_found;
        if (outfile) {
            struct timespec tick;
            clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
            u64 execution_time_ms = (tick.tv_sec - start.tv_sec)*1000 +
                                    (tick.tv_nsec - start.tv_nsec) / 1000000;
            double npms = (double)nodes_searched / (double) execution_time_ms;
            double nps = npms * 1000;
            fprintf(outfile, "info depth %i score cp %i nodes %llu nps %i\n",
                    ply_depth + 1,
                    best_score_found,
                    (unsigned long long) nodes_searched,
                    (int) nps);
        }
        ply_depth++;
    }
}

Centipawns qsearch(Board *board, Centipawns alpha, Centipawns beta,
                   _Atomic (bool) *stop, u64 *nodes_searched) {
    (*nodes_searched)++;
    int stand_pat = evaluation(board, board->_turn);
    if (stand_pat >= beta) {
        return beta;
    }
    if (alpha < stand_pat) {
        alpha = stand_pat;
    }
    MoveList capture_moves = generate_capture_moves(board);
    for (int i = 0; i < capture_moves.count; i++) {
        if (*stop) {
            break;
        }
        Move mv = move_list_get(&capture_moves, i);
        board_make_move(board, mv);
        Centipawns score = -qsearch(board, -beta, -alpha, stop, nodes_searched);
        board_unmake(board);
        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

Centipawns search_recursive(Board *board, Centipawns alpha, Centipawns beta,
                            i32 ply_depth, _Atomic (bool) *stop, Move *pv_move, u64 *nodes_searched) {
    (*nodes_searched)++;
    if (ply_depth == 0) {
        return qsearch(board, alpha, beta,
                       stop, nodes_searched); // evaluation(board, board->_turn);
    }
    MoveList moves = generate_all_legal_moves(board);
    Move best_move_found;
    for (int i = 0; i < moves.count; i++) {
        if (*stop) {
            // we don't care about the result here
            break;
        }
        Move mv = move_list_get(&moves, i);
        board_make_move(board, mv);
        Centipawns score =
                -search_recursive(board, -beta, -alpha, ply_depth - 1, stop, pv_move, nodes_searched);
        board_unmake(board);
        if (score >= beta) {
            (*pv_move) = best_move_found; // do we need? we're not in a PV-node
            // this is a Cut-node
            // we return a lower bound; the exact score might be higher
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            best_move_found = mv;
        }
    }
    (*pv_move) = best_move_found;
    return alpha;
}
