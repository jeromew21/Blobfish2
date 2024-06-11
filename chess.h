#pragma once
#include <stdint.h>
#include <stdio.h>

/* https://www.chessprogramming.org/Square_Mapping_Considerations#LittleEndianRankFileMapping */
typedef uint64_t u64;

/* https://www.chessprogramming.org/Encoding_Moves */
typedef uint16_t Move; // could using 32 bits instead be more optimized?

typedef uint32_t u32;

typedef int32_t  i32;

/**
 * See note on MoveList for more information.
 */
#define MOVELIST_STACK_COUNT 256

/** 
 * 5899 is theoretical max length of chess game, with 50 move rule 
 * and 3fold repetition taken into account.
 */
#define MAX_BOARD_STACK_DEPTH 6000

enum Piece {
    kWhite,
    kBlack,
    kPawn,
    kBishop,
    kRook,
    kQueen,
    kKnight,
    kKing,
};

/**
 * These are stored in the stack.
 * We really want to shrink the size of this for ease of use.
 * Looks like we could make it a single 32-bit integer.
 */
typedef struct BoardMetadata {
    Move _last_move; // 16 bits for now
    i32 _en_passant_square; // 8 bits?
    i32 _captured_piece; // 4 bits indexing into bitboard ?
    uint8_t _castling_rights[4]; // should be a 4-bit field
} BoardMetadata;

/**
 * This only holds the minimal data for a board state.
 * Dense bitboard structure with 2 major bitboards then each piece.
 * https://www.chessprogramming.org/Bitboard_Board-Definition
 */
typedef struct Board {
    u64 _bitboard[8];
    i32 _turn;
    i32 _ply;
    BoardMetadata _state_stack[MAX_BOARD_STACK_DEPTH];
} Board;

/**
 * These values make up the top 4 bits of a Move, encoding special move types.
 * I'm pretty sure that in my previous engines I didn't encode Capture, but there might be some utility here to do so.
 */
enum MoveMetadata {
    kQuietMove,
    kCaptureMove,
    kDoublePawnMove,
};

/**
 * This is a mutable list of moves, generated on the fly for a particular board state.
 * We will store a small amount of moves on the stack (TODO: all of them?)
 * Right now, that's all of them, but since the realistic branching factor for chess is only about ~35,
 * we are wasting valuable(?) stack space.
 * Note that the max moves per position is theoretically 218.
 */
typedef struct MoveList {
    int count;
    Move _stack_data[MOVELIST_STACK_COUNT];
    //Move* _data;
    //int _data_capacity;
} MoveList;

/* Debug */

void dump_u64(u64 bitset);

void dump_board(Board* board);

/* Bit Twiddling */

u32 bitscan_forward(u64 bitset);

u32 bitscan_reverse(u64 bitset);

u32 popcount(u64 bitset);

u64 permute_mask(u64 mask, i32 index, i32 pop_count);

/* Move */

Move move_create(u32 src, u32 dest, u32 flags);

u64 move_get_dest(Move mv);

u64 move_get_src(Move mv);

u32 move_get_dest_u32(Move mv);

u32 move_get_src_u32(Move mv);

u32 move_get_metadata(Move mv);

/* Move List */

MoveList move_list_create();

Move move_list_get(MoveList *list, i32 index);

void move_list_push(MoveList *list, Move mv);

/* Move Generation */

MoveList generate_all_pseudo_legal_moves(Board* board);

/* Board Modifiers*/

void make_move(Board* board, Move mv);

void unmake(Board* board);