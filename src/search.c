#include "search.h"
#include "chess.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TranspositionTable tt;
KillerTable killer_table;

TTableBucket *ttable_probe(u64 hash); // TODO

typedef struct SearchArguments {
    Board *board;
    Centipawns alpha;
    Centipawns beta;
    i32 ply_depth;
    AtomicBool *stop;
    u64 *nodes_searched;
} SearchArguments;

typedef struct ScoredMove {
    Move mv;
    i32 score;
    bool valid;
} ScoredMove;

typedef struct ScoredMoveList {
    i32 count;
    ScoredMove items[256];
} ScoredMoveList;

Centipawns search_recursive(SearchArguments args);

Centipawns qsearch(Board *board, Centipawns alpha, Centipawns beta,
                   AtomicBool *stop);

Move pop_max(ScoredMoveList *scored_moves);

Move peek_max(ScoredMoveList *scored_moves);

Centipawns min_cp(Centipawns x, Centipawns y) { return x < y ? x : y; }

Centipawns max_cp(Centipawns x, Centipawns y) { return x > y ? x : y; }

/**
 * Root search
 * TODO: put depth limit on as well
 */
void search(Board *board, Move *best_move, AtomicBool *stop_thinking,
            FILE *outfile, int depth_limit) {
    // TODO: multi threading
    // TODO: allow search subset of moves (this is a UCI requirement but rarely seen)
    // TODO: don't return best move in recursive impl, use root node search
    // TODO: do we alpha-beta prune at root? why or why not?
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    MoveList legal_moves = generate_all_legal_moves(board);
    int ply_depth = 0;
    u64 nodes_searched = 0;
    ScoredMoveList scored_moves;
    scored_moves.count = 0;
    u64 hash = board_metadata_peek(board, 0)->_hash;
    TTableBucket *bucket_ptr = ttable_probe(hash);
    Move tt_move = 0;
    if (bucket_ptr->hash == hash) {
        tt_move = bucket_ptr->best_move;
    }
    for (i32 i = 0; i < legal_moves.count; i++) {
        Move mv = move_list_get(&legal_moves, i);
        u32 mv_md = move_get_metadata(mv);
        u64 src = move_get_src(mv);
        i32 score = 0;
        if (mv == tt_move) {
            score += 1000;
        }
        if (mv_md & PROMOTION_BIT_FLAG) {
            score += 100;
        }
        if (mv_md & CAPTURE_BIT_FLAG) {
            score += 100;
        }
        if (src & board->_bitboard[kPawn]) {
            score += 10;
        }
        scored_moves.items[scored_moves.count].mv = mv;
        scored_moves.items[scored_moves.count].score = score;
        scored_moves.items[scored_moves.count].valid = true;
        scored_moves.count++;
    }
    (*best_move) = peek_max(&scored_moves);
    while (1) {
        if (*stop_thinking)
            return;
        Centipawns best_score_found = MIN_EVAL;
        Centipawns alpha = MIN_EVAL;
        Centipawns beta = -MIN_EVAL;
        ScoredMoveList scored_moves_copy = scored_moves;
        Move best_move_found = peek_max(&scored_moves_copy);
        for (int i = 0; i < legal_moves.count; i++) {
            if (*stop_thinking)
                return;
            Move mv = pop_max(&scored_moves_copy);
            board_make_move(board, mv);
            SearchArguments sub_args;
            sub_args.board = board;
            sub_args.alpha = -beta;
            sub_args.beta = -alpha;
            sub_args.ply_depth = ply_depth;
            sub_args.stop = stop_thinking;
            sub_args.nodes_searched = &nodes_searched;
            Centipawns score = -search_recursive(sub_args);
            if (score > best_score_found) {
                best_score_found = score;
                best_move_found = mv;
            }
            board_unmake(board);
            for (int k = 0; k < legal_moves.count; k++) {
                if (scored_moves.items[k].mv == mv) {
                    scored_moves.items[k].score = score;
                    break;
                }
            }
        }
        if (*stop_thinking)
            return;
        (*best_move) = best_move_found;
        char score_string[64];
        bool mate = false;
        if (abs(MIN_EVAL) - abs(best_score_found) < 1024) {
            // 1024 leaves room for, say, mate in 30, with a large game of around
            // 400+ plies. mate in n now plies = distance from board 0th ply to
            // leaf.depth
            if (best_score_found > 0) {
                int plies = -MIN_EVAL - best_score_found;
                int moves_to_mate = (int)ceil(((double) (plies - board->_ply)) / 2.0);
                sprintf(score_string, "mate %i", moves_to_mate);
            } else {
                int plies = best_score_found - MIN_EVAL;
                int moves_to_mate = (int)ceil(((double) (plies - board->_ply)) / 2.0);
                sprintf(score_string, "mate -%i", moves_to_mate);
            }
            mate = true;
        } else {
            sprintf(score_string, "cp %i", best_score_found);
        }
        struct timespec tick;
        clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
        u64 execution_time_ms = (tick.tv_sec - start.tv_sec) * 1000 +
                                (tick.tv_nsec - start.tv_nsec) / 1000000;
        if (execution_time_ms == 0) {
            execution_time_ms = 1;
        }
        double npms = (double) nodes_searched / (double) execution_time_ms;
        double nps = npms * 1000.;
        double hashfull = 1000. * (double) tt.filled / (double) tt.count;
        char pv[8192];
        pv[0] = '\0';
        {
            Move mv = best_move_found;
            char buf[16];
            int i = 0;
            while (i < ply_depth + 1) {
                MoveList legals = generate_all_legal_moves(board);
                bool mv_is_legal = false;
                for (int k = 0; k < legals.count; k++) {
                    if (mv == move_list_get(&legals, k)) {
                        mv_is_legal = true;
                        break;
                    }
                }
                if (!mv_is_legal) {
                    break;
                }
                move_to_string(mv, buf);
                board_make_move(board, mv);
                sprintf(pv + strlen(pv), " %s", buf);
                i++;
                TTableBucket *bucket =
                        ttable_probe(board_metadata_peek(board, 0)->_hash);
                if (bucket->hash == 0) {
                    break;
                }
                if (bucket->node_type != kPV) {
                    break;
                }
                mv = bucket->best_move;
            }
            for (int k = 0; k < i; k++) {
                board_unmake(board);
            }
        }
        if (outfile) {
            fprintf(outfile,
                    "info depth %i score %s nodes %llu nps %i hashfull %i time %i pv%s\n",
                    ply_depth + 1, score_string, (unsigned long long) nodes_searched,
                    (int) nps, (int) hashfull, (int) execution_time_ms, pv);
        }
        if (mate || ply_depth > 2048 || ply_depth >= depth_limit) {
            // there's a bug here, sometimes it doesn't return
            // it bugs out and even sometimes hangs GUI
            // rn, we have a constant check, but this isn't correct
            // i think the bug source is, a threefold repetition is evaluated as 0, but
            // the actual board legal moves continues to grow and we get a stupidly long PV.
            // we need to stop generating moves after a threefold, basically. This would be easy to implement.
            // or treat it similarly to mate (stop searching)
            return;
        }
        ply_depth++;
    }
}

/**
 * Quiescience search
 */
Centipawns qsearch(Board *board, Centipawns alpha, Centipawns beta,
                   AtomicBool *stop) {
    int stand_pat = evaluation(board);
    if (stand_pat >= beta) {
        return beta;
    }
    if (alpha < stand_pat) {
        alpha = stand_pat;
    }
    MoveList capture_moves = generate_capture_moves(board);
    for (int i = 0; i < capture_moves.count; i++) {
        if (*stop) {
            return alpha;
        }
        Move mv = move_list_get(&capture_moves, i);
        board_make_move(board, mv);
        Centipawns score = -qsearch(board, -beta, -alpha, stop);
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

/**
 * Our workhorse Alpha-Beta Search
 * TODO: PVS
 */
Centipawns search_recursive(SearchArguments args) {
    (*args.nodes_searched)++;
    Move tt_move = 0;
    u64 hash = board_metadata_peek(args.board, 0)->_hash;
    TTableBucket *bucket_ptr = ttable_probe(hash);
    TTableBucket bucket_prev = *bucket_ptr;
    TTableBucket bucket = *bucket_ptr;
    if (bucket.hash == hash) {
        tt_move = bucket.best_move;
        if (bucket.depth >= args.ply_depth) {
            switch (bucket.node_type) {
                case kCut:
                    args.alpha = max_cp(args.alpha, bucket.score);
                    break;
                case kAll:
                    args.beta = min_cp(args.beta, bucket.score);
                    break;
                case kPV: {
                    return bucket.score;
                }
            }
            if (args.alpha >= args.beta) {
                return args.beta;
            }
            // TODO: pros/cons of returning or continuing here
            //return bucket.score;
        }
    }
    i32 status = board_status(args.board);
    if (status == kCheckmate) {
        Centipawns mating_score = MIN_EVAL + (i32) args.board->_ply;
        return mating_score;
    }
    if (status == kStalemate || status == kDraw) {
        return 0; // TODO: contempt factor
    }
    if (args.ply_depth == 0) {
        Centipawns qscore = qsearch(args.board, args.alpha, args.beta, args.stop);
        return qscore;
    }
    // WHY DO WE START INSERTING AFTER HERE???
    // do we want to store leaf results??
    if (bucket.hash == 0) {
        tt.filled += 1;
    }
    bucket.hash = hash;
    bucket.depth = (u8)args.ply_depth;
    MoveList legal_moves = generate_all_legal_moves(args.board);
    ScoredMoveList scored_moves;
    scored_moves.count = 0;
    for (i32 i = 0; i < legal_moves.count; i++) {
        Move mv = move_list_get(&legal_moves, i);
        u32 mv_md = move_get_metadata(mv);
        u64 src = move_get_src(mv);
        i32 score = 0;
        u64 masked_killer = ((u64) mv) & killer_table.mask;
        KillerTableBucket *killer_bucket = &killer_table.buckets[masked_killer];
        if (killer_bucket->root_distance == args.board->_ply && killer_bucket->mv == mv) {
            score += 101;
        }
        if (mv == tt_move) {
            score += 1000;
        }
        if (mv_md & PROMOTION_BIT_FLAG) {
            score += 100;
        }
        if (mv_md & CAPTURE_BIT_FLAG) {
            score += 100;
        }
        if (src & args.board->_bitboard[kPawn]) {
            score += 10;
        }
        scored_moves.items[scored_moves.count].mv = mv;
        scored_moves.items[scored_moves.count].score = score;
        scored_moves.items[scored_moves.count].valid = true;
        scored_moves.count++;
    }
    bucket.best_move = peek_max(&scored_moves);
    bucket.node_type = kAll; // Default is all-node, an upper bound (exact score might be lower)
    for (int i = 0; i < legal_moves.count; i++) {
        if (*args.stop) {
            return args.alpha;
        }
        Move mv = pop_max(&scored_moves);
        board_make_move(args.board, mv);
        SearchArguments sub_args;
        sub_args.board = args.board;
        sub_args.alpha = -args.beta;
        sub_args.beta = -args.alpha;
        sub_args.ply_depth = args.ply_depth - 1;
        sub_args.stop = args.stop;
        sub_args.nodes_searched = args.nodes_searched;
        Centipawns score = -search_recursive(sub_args);
        board_unmake(args.board);
        if (score >= args.beta) {
            // this is a Cut-node
            // we return a lower bound; the exact score might be higher
            bucket.node_type = kCut;
            bucket.best_move = mv;
            args.alpha = args.beta;
            if (!(move_get_metadata(mv) & CAPTURE_BIT_FLAG)) {
                u64 masked_killer = ((u64) mv) & killer_table.mask;
                killer_table.buckets[masked_killer].mv = mv;
                killer_table.buckets[masked_killer].root_distance = args.board->_ply;
            }
            break;
        }
        if (score > args.alpha) {
            bucket.node_type = kPV;
            bucket.best_move = mv;
            args.alpha = score;
        }
    }
    bucket.score = args.alpha;
    bool eviction_cond = (bucket_prev.node_type != kPV || bucket_prev.hash == 0) && (bucket_prev.depth <= bucket.depth);
    if (eviction_cond) {
        (*bucket_ptr) = bucket;
    }
    return args.alpha;
}

Move pop_max(ScoredMoveList *scored_moves) {
    ScoredMove best;
    best.score = -100000000;
    best.mv = 0;
    int best_idx = 0;
    for (i32 i = 0; i < scored_moves->count; i++) {
        ScoredMove sm = scored_moves->items[i];
        if (!sm.valid) {
            continue;
        }
        if (sm.score > best.score) {
            best = sm;
            best_idx = i;
        }
    }
    scored_moves->items[best_idx].valid = false;
    return best.mv;
}

Move peek_max(ScoredMoveList *scored_moves) {
    ScoredMove best;
    best.score = -100000000;
    best.mv = 0;
    for (i32 i = 0; i < scored_moves->count; i++) {
        ScoredMove sm = scored_moves->items[i];
        if (!sm.valid) {
            continue;
        }
        if (sm.score > best.score) {
            best = sm;
        }
    }
    return best.mv;
}

TTableBucket *ttable_probe(u64 hash) {
    TTableBucket *bucket = &tt.buckets[hash & tt.mask];
    return bucket;
}

// TODO: add parameters
void init_tables(void) {
    {
        const u64 target_table_size_mib = 16; // 1mib = 1024 x 1024 bytes
        const u64 target_table_size_b = target_table_size_mib * 1024 * 1024;
        const u64 bucket_size_b = sizeof(TTableBucket);
        int n = 0;
        u64 count;
        while (1) {
            count = (u64) 1 << n;
            const u64 table_size_b = count * bucket_size_b;
            if (table_size_b > target_table_size_b) {
                count = (u64) 1 << (n - 1);
                break;
            }
            n++;
        }
        tt.count = count;
        tt.filled = 0;
        tt.buckets = calloc(1, count * bucket_size_b);
        tt.mask = count - 1;
        printf("info transposition table size %i mib\n",
               (int) (count * bucket_size_b / (1024 * 1024)));
        printf("info transposition table count %llu buckets\n",
               (long long unsigned) tt.count);
    }
    {
        killer_table.count = (u64) 1 << 16;
        killer_table.mask = killer_table.count - 1;
        killer_table.buckets = calloc(1, sizeof(KillerTableBucket) * killer_table.count);
    }
}

void destroy_tables(void) {
    free(tt.buckets);
    free(killer_table.buckets);
}
