#include "uci.h"
#include "chess.h"
#include "parse.h"
#include "test.h"
#include "search.h"
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *ENGINE_NAME = "Blobfish2";

static const char *ENGINE_VERSION = "0.0.1";

EngineContext *ctx;

typedef void (*cmd_func)(char *);

bool board_make_move_from_alg(Board *board, const char *algebraic);

void command_uci(char *line_buffer);

void command_debug(char *line_buffer);

void command_isready(char *line_buffer);

void command_setoption(char *line_buffer);

void command_go(char *line_buffer);

void command_stop(char *line_buffer);

void command_perft(char *line_buffer);

void command_ucinewgame(char *line_buffer);

void command_position(char *line_buffer);

void command_quit(char *line_buffer);

void command_test(char *line_buffer);

void command_dump(char *line_buffer);

void command_help(char *line_buffer);

void engine_command(char *line_buffer) {
#define COMMAND_COUNT 11
    static char *commands[COMMAND_COUNT] = {
            "quit", "test", "uci", "perft", "position", "ucinewgame",
            "isready", "go", "stop", "dump", "help"};
    static const cmd_func command_functions[COMMAND_COUNT] = {
            command_quit, command_test, command_uci, command_perft,
            command_position, command_ucinewgame, command_isready, command_go,
            command_stop, command_dump, command_help};

    fprintf(ctx->log_fp, "INFO: GUI command `%.*s`\n",
            (int) strlen(line_buffer) - 1, line_buffer);
    fflush(ctx->log_fp);

    char command_name[64];
    int i = 0;
    eat_whitespace(line_buffer, &i);
    eat_word(line_buffer, command_name, &i);
    for (int k = 0; k < COMMAND_COUNT; k++) {
        if (strings_equal(command_name, commands[k])) {
            command_functions[k](line_buffer + i);
            return;
        }
    }
    printf("Unknown command `%s`.\n", command_name);
#undef COMMAND_COUNT
}

void *think_timer(void *_unused) {
    const u64 think_ms = (u64) ctx->think_time_ms - 1; // buffer by 1 ms... enough?
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    while (1) {
        struct timespec tick;
        clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
        u64 execution_time_ms = (tick.tv_sec - start.tv_sec) * 1000 +
                                (tick.tv_nsec - start.tv_nsec) / 1000000;
        if (execution_time_ms >= think_ms) {
            ctx->stop_thinking = true;
            break;
        }
        if (ctx->stop_thinking) {
            break;
        }
    }
    return NULL;
}

void *think(void *_unused) {
    search(ctx->board, &ctx->best_move, &ctx->stop_thinking, stdout);
    char move_buf[16];
    move_to_string(ctx->best_move, move_buf);
    printf("bestmove %s\n", move_buf);
    ctx->stop_thinking = true;
    return NULL;
}

void command_go(char *line_buffer) {
    enum number_args {
        kWTime, kBTime, kMovesToGo, kMoveTime
    };
    if (!ctx->stop_thinking) // ignore if already going
        return;
    ctx->stop_thinking = false;
    i32 arguments[4] = {-1, -1, -1, -1};
    int i = 0;
    char word_buffer[64];
    char arg_buffer[64];
    // TODO: this is not perfect, but works?
    while (eat_word(line_buffer, word_buffer, &i)) {
        if (strings_equal("infinite", word_buffer)) {
            // well,,, technically, not infinite...
            break;
        } else if (strings_equal("wtime", word_buffer)) {
            eat_word(line_buffer, arg_buffer, &i);
            arguments[kWTime] = atoi(arg_buffer);
        } else if (strings_equal("btime", word_buffer)) {
            eat_word(line_buffer, arg_buffer, &i);
            arguments[kBTime] = atoi(arg_buffer);
        } else if (strings_equal("movetime", word_buffer)) {
            eat_word(line_buffer, arg_buffer, &i);
            arguments[kMoveTime] = atoi(arg_buffer);
        } else if (strings_equal("movestogo", word_buffer)) {
            eat_word(line_buffer, arg_buffer, &i);
            arguments[kMovesToGo] = atoi(arg_buffer);
        }
    }
    f64 moves_to_go = arguments[kMovesToGo] > 0 ? arguments[kMovesToGo] : 10;
    if (ctx->board->_turn == kWhite && arguments[kWTime] > 0) {
        ctx->think_time_ms = (f64) arguments[kWTime] / moves_to_go;
    } else if (ctx->board->_turn == kBlack && arguments[kBTime] > 0) {
        ctx->think_time_ms = (f64) arguments[kBTime] / moves_to_go;
    } else if (arguments[kMoveTime] > 0) {
        ctx->think_time_ms = (f64) arguments[kMoveTime];
    } else {
        ctx->think_time_ms = FLT_MAX;
    }
    //printf("info decided to think for %i ms\n", (int) ctx->think_time_ms);
    ctx->stop_thinking = false;
    THREAD think_timer_thread;
    THREAD_CREATE(&think_timer_thread, NULL, think_timer, (void *) NULL);
    THREAD_DETACH(think_timer_thread);
    THREAD think_thread;
    THREAD_CREATE(&think_thread, NULL, think, (void *) NULL);
    THREAD_DETACH(think_thread);
}

void stop_searching(void) {
    if (!ctx->stop_thinking) {
        ctx->stop_thinking = true;
    }
}

void command_stop(char *line_buffer) { stop_searching(); }

void command_perft(char *line_buffer) {
    // TODO: perft command
    // printf("perft %s\n", line_buffer);
}

void command_ucinewgame(char *line_buffer) {
    // not sure if this is meaningful for us
    // maybe the idea is that we clear hash tables and any other saved state from
    // the previous game
    // probably safe to ignore
}

void command_isready(char *line_buffer) {
    // This might be called if there's a blocking operation on the entire engine.
    // Given example is loading tablebases.
    printf("readyok\n");
}

void command_quit(char *line_buffer) { ctx->quit = true; }

void command_position(char *line_buffer) {
    int i = 0;
    memset(ctx->board, 0, sizeof(Board));
    char word[16];
    eat_word(line_buffer, word, &i);
    if (strings_equal("fen", word)) {
        fen_parse(ctx->board, line_buffer, &i);
    } else {
        board_initialize_startpos(ctx->board);
    }
    eat_word(line_buffer, word, &i);
    if (strings_equal("moves", word)) {
        while (eat_word(line_buffer, word, &i)) {
            board_make_move_from_alg(ctx->board, word);
        }
    }
}

void command_uci(char *line_buffer) {
    printf("id name %s %s\n", ENGINE_NAME, ENGINE_VERSION);
    printf("id author Jerome Wei\n");
    printf("option name Foo type check default false\n");
    printf("uciok\n");
}

void command_dump(char *line_buffer) { board_dump(ctx->board); }

void command_help(char *line_buffer) {
    printf("%s is a simple chess engine made for educational purposes.\n",
           ENGINE_NAME);
    printf("See UCI protocol for usage information.\n");
    printf("License and source code available TODO.\n");
}

void command_test(char *line_buffer) {
    int i = 0;
    char word_buffer[64];
    while (eat_word(line_buffer, word_buffer, &i)) {
        if (strings_equal("perft", word_buffer)) {
            printf("Testing from file...\n");
            perft_test_from_file("./test/standard.epd", 8);
        } else if (strings_equal("performance", word_buffer) || strings_equal("perf", word_buffer)) {
            perft_performance_test();
        } else if (strings_equal("puzzles", word_buffer)) {
            puzzle_test("./test/lichess_db_puzzle.csv");
        }
    }
}

EngineContext *engine_get_context(void) { return ctx; }

void engine_initialize(void) {
    ctx = malloc(sizeof(EngineContext));
    ctx->board = calloc(1, sizeof(Board));
    board_initialize_startpos(ctx->board);
    ctx->stop_thinking = true;
    ctx->debug = false;
    ctx->quit = false;
    ctx->log_fp = fopen("log.txt", "a");
    fprintf(ctx->log_fp, "INFO: started new %s instance\n", ENGINE_NAME);
}

void engine_cleanup(void) {
    free(ctx->board);
    fclose(ctx->log_fp);
    free(ctx);
}

/**
 * Given an algebraic move like "a2b1q", make the move on board.
 * This shouldn't be called within a loop.
 */
bool board_make_move_from_alg(Board *board, const char *algebraic) {
    static char row_names[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    static char col_names[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    u32 r0, c0, r1, c1;
    for (u32 i = 0; i < 8; i++) {
        if (algebraic[0] == col_names[i])
            c0 = i;
        if (algebraic[1] == row_names[i])
            r0 = i;
        if (algebraic[2] == col_names[i])
            c1 = i;
        if (algebraic[3] == row_names[i])
            r1 = i;
    }
    i32 promotion_mask = -1;
    if (strlen(algebraic) == 5) {
        switch (algebraic[4]) {
            case 'q':
                promotion_mask = 3;
                break;
            case 'n':
                promotion_mask = 0;
                break;
            case 'b':
                promotion_mask = 1;
                break;
            case 'r':
                promotion_mask = 2;
                break;
            default:
                printf("failed to find alg\n");
                exit(44);
        }
    }
    u32 idx_src = r0 * 8 + c0;
    u32 idx_dest = r1 * 8 + c1;
    MoveList moves = generate_all_legal_moves(board);
    Move mv;
    bool found = false;
    for (int i = 0; i < moves.count; i++) {
        Move candidate = move_list_get(&moves, i);
        u32 candidate_src = move_get_src_u32(candidate);
        u32 candidate_dest = move_get_dest_u32(candidate);
        if (idx_src == candidate_src && idx_dest == candidate_dest) {
            if (promotion_mask == -1) {
                mv = candidate;
                found = true;
                break;
            } else {
                u32 md = move_get_metadata(candidate);
                if (md & PROMOTION_BIT_FLAG) {
                    if ((md & 0x3) == (u32) promotion_mask) {
                        mv = candidate;
                        found = true;
                        break;
                    }
                }
            }
        }
    }
    if (!found) {
        exit(23);
    }
    board_make_move(board, mv);
    return true;
}

