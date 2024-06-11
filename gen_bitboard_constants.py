import datetime
FILENAME = "bitboard_constants.h"

def in_bounds(x):
    return 0 <= x < 8

def write_prelude(f):
    f.write("""/* DO NOT EDIT BY HAND. AUTOGENERATED BY PYTHON AT {} */
#pragma once
#include "chess.h"

""".format(datetime.datetime.now()))

def write_rook_moves(f):
    f.write("static const u64 BITBOARD_ROOK_RAYS[4][64] = {\n")
    for (x, y) in ((0, 1), (1, 0), (0, -1), (-1, 0)):
        f.write("\t{\n")
        for source in range(64):
            board = [0 for _ in range(64)]
            col = source % 8
            row = source // 8
            while True:
                col += x
                row += y 
                if in_bounds(col) and in_bounds(row):
                    flat = row*8 + col
                    board[flat] = 1
                else:
                    break
            board[source] = 0
            f.write("\t\t0b")
            for i in reversed(board):
                f.write(f"{i}")
            f.write(",\n")          
        f.write("\t},\n")
    f.write("};\n\n")

def write_bishop_moves(f):
    f.write("static const u64 BITBOARD_BISHOP_RAYS[4][64] = {\n")
    for (x, y) in ((-1, 1), (1, 1), (1, -1), (-1, -1)):
        f.write("\t{\n")
        for source in range(64):
            board = [0 for _ in range(64)]
            col = source % 8
            row = source // 8
            while True:
                col += x
                row += y 
                if in_bounds(col) and in_bounds(row):
                    flat = row*8 + col
                    board[flat] = 1
                else:
                    break
            board[source] = 0
            f.write("\t\t0b")
            for i in reversed(board):
                f.write(f"{i}")
            f.write(",\n")          
        f.write("\t},\n")
    f.write("};\n\n")


def write_king_moves(f):
    f.write("static const u64 BITBOARD_KING_ATTACKS[64] = {\n")
    for source in range(64):
        board = [0 for _ in range(64)]
        row = source // 8
        col = source % 8
        d = (-1, 1, 0)
        for x in d:
            for y in d:
                rp = row + x
                cp = col + y
                if in_bounds(rp) and in_bounds(cp):
                    flat = rp*8 + cp
                    board[flat] = 1
        board[source] = 0
        f.write("\t0b")
        for i in reversed(board):
            f.write(f"{i}")
        f.write(",\n")          
    f.write("};\n\n")

def write_knight_moves(f):
    f.write("static const u64 BITBOARD_KNIGHT_ATTACKS[64] = {\n")
    for source in range(64):
        board = [0 for _ in range(64)]
        row = source // 8
        col = source % 8
        d = (-1, 1, 2, -2)
        for x in d:
            for y in d:
                if abs(x) != abs(y):
                    rp = row + x
                    cp = col + y
                    if in_bounds(rp) and in_bounds(cp):
                        flat = rp*8 + cp
                        board[flat] = 1
        board[source] = 0
        f.write("\t0b")
        for i in reversed(board):
            f.write(f"{i}")
        f.write(",\n")          
    f.write("};\n\n")

if __name__ == "__main__":
    with open(FILENAME, "w") as f:
        write_prelude(f)
        write_knight_moves(f)
        write_king_moves(f)
        write_bishop_moves(f)
        write_rook_moves(f)
