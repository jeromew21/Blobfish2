cmake_minimum_required(VERSION 3.23)
project(Blobfish
        VERSION 1.0
        LANGUAGES C)
add_executable(blobfish
        src/bit_twiddling.c
        src/fen_parse.c
        src/debug.c
        src/perft.c
        src/move.c
        src/move_generation.c
        src/move_list.c
        src/move_make_unmake.c
        src/chess.c)
