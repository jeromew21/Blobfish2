#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* https://www.chessprogramming.org/Square_Mapping_Considerations#LittleEndianRankFileMapping
 */
typedef uint64_t u64;

/* https://www.chessprogramming.org/Encoding_Moves */
typedef uint16_t Move; // could using 32 bits instead be more optimized?

typedef uint32_t u32;

typedef int32_t i32;

typedef double f64;

static const i32 PROMOTION_BIT_FLAG = 0x8;

static const i32 CAPTURE_BIT_FLAG = 0x4;

/**
 * See note on MoveList for more information.
 */
#define MOVELIST_STACK_COUNT 256

/**
 * 5899 is theoretical max length of chess game, with 50 move rule
 * and 3fold repetition taken into account.
 */
#define MAX_BOARD_STACK_DEPTH 6000

/**
 * Our pieces which index into our bitboards
 * DO NOT TOUCH!!!
 */
enum Piece {
  kWhite = 0,
  kBlack = 1,
  kPawn = 2,
  kBishop = 3,
  kKnight = 4,
  kRook = 5,
  kQueen = 6,
  kKing = 7,
};

/**
 * 0b0000 means you are allowed to castle (with legal castling board state; i.e.
 * king and rook haven't moved yet). 1 in any slot means it's not allowed.
 */
enum CastlingRights {
  kWhiteKingSideFlag = 0x1,  // 0b0001,
  kWhiteQueenSideFlag = 0x2, // 0b0010,
  kBlackKingSideFlag = 0x4,  // 0b0100,
  kBlackQueenSideFlag = 0x8, // 0b1000,
};

/**
 * These are stored in the stack.
 * We really want to shrink the size of this for ease of use.
 * Looks like we could make it a single 32-bit integer.
 */
typedef struct BoardMetadata {
  Move _last_move; // 16 bits for now
  uint16_t _state_data;
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
  u64 _rook_start_positions;
  BoardMetadata _initial_state;
  BoardMetadata _state_stack[MAX_BOARD_STACK_DEPTH];
} Board;

/**
 * These values make up the top 4 bits of a Move, encoding special move types.
 * I'm pretty sure that in my previous engines I didn't encode Capture, but
 * there might be some utility here to do so. Captures end up having a bit flag
 * with this encoding. https://www.chessprogramming.org/Encoding_Moves
 * Note that 0b0100 masks for captures, and 0b1000 masks for promotions.
 */
enum MoveMetadata {
  kQuietMove = 0x0,
  kDoublePawnMove = 0x1,             // 0b0001,
  kKingSideCastleMove = 0x2,         // 0b0010,
  kQueenSideCastleMove = 0x3,        // 0b0011,
  kCaptureMove = 0x4,                // 0b0100,
  kEnPassantMove = 0x5,              // 0b0101,
  kKnightPromotionMove = 0x8,        // 0b1000,
  kKnightCapturePromotionMove = 0xc, // 0b1100,
  kBishopPromotionMove = 0x9,        // 0b1001,
  kBishopCapturePromotionMove = 0xd, // 0b1101,
  kRookPromotionMove = 0xa,          // 0b1010,
  kRookCapturePromotionMove = 0xe,   // 0b1110,
  kQueenPromotionMove = 0xb,         // 0b1011,
  kQueenCapturePromotionMove = 0xf,  // 0b1111,
};

/**
 * This is a mutable list of moves, generated on the fly for a particular board
 * state. We will store a small amount of moves on the stack (TODO: all of
 * them?) Right now, that's all of them, but since the realistic branching
 * factor for chess is only about ~35, we are wasting valuable(?) stack space.
 * Note that the max moves per position is theoretically 218, so 256 makes sense
 * as an upper bound.
 */
typedef struct MoveList {
  int count;
  Move _stack_data[MOVELIST_STACK_COUNT];
  // Move* _data;
  // int _data_capacity;
} MoveList;

/* Debug */

void dump_u64(u64 bitset);

void dump_board(Board *board);

void dump_bitboard(u64 *bitboards);

void move_to_string(Move mv, char *buf);

/* Bit Twiddling */

u32 bitscan_forward(u64 bitset);

u32 bitscan_reverse(u64 bitset);

i32 pop_count(u64 bitset);

// u64 permute_mask(u64 mask, i32 index, i32 pop_count);

/* Move */

Move move_create(u32 src, u32 dest, u32 flags);

u64 move_get_dest(Move mv);

u64 move_get_src(Move mv);

u32 move_get_dest_u32(Move mv);

u32 move_get_src_u32(Move mv);

u32 move_get_metadata(Move mv);

/* Move List */

MoveList move_list_create(void);

Move move_list_get(MoveList *list, i32 index);

void move_list_push(MoveList *list, Move mv);

// void move_list_destroy

/* Move Generation */

MoveList generate_all_legal_moves(Board *board);

MoveList generate_capture_moves(Board *board);

void bitboards_update(u64 *bitboards, i32 turn, Move mv);

/* Board Metadata */

BoardMetadata *board_metadata_peek(Board *board, int i);

void board_metadata_set_en_passant_square(BoardMetadata *md, u32 ep_square);

void board_metadata_set_captured_piece(BoardMetadata *md, u32 captured_piece);

void board_metadata_set_castling_rights(BoardMetadata *md, u32 rights);

u32 board_metadata_get_en_passant_square(BoardMetadata *md);

u32 board_metadata_get_captured_piece(BoardMetadata *md);

u32 board_metadata_get_castling_rights(BoardMetadata *md);

/* Board construction */

Board *board_uninitialized(void);

void board_initialize_fen(Board *board, const char *fen);

void board_initialize_startpos(Board *board);

void fen_parse(Board *board, const char *fen, i32 *i);

/* Board state */

bool board_is_check(Board *board); // TODO

i32 board_status(Board *board); // TODO: checkmate, stalemate, 50 move, etc.

/* Board Modifiers*/

void board_make_move(Board *board, Move mv);

void board_unmake(Board *board);

/* Piece movement */

i32 attacker_count(u64 bitset, u64 *bitboards, i32 attacking_color);

bool is_attacked(u64 bitset, u64 *bitboards, i32 attacking_color);

u64 king_moves(u32 source_idx);

u64 knight_moves(u32 source_idx);

u64 bishop_moves(u32 source_idx, u64 occupancy_mask);

u64 rook_moves(u32 source_idx, u64 occupancy_mask);

u64 pawn_attacks(u64 source_bitset, i32 side);
