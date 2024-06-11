cmake_minimum_required(VERSION 3.23)
project(Blobfish
        VERSION 1.0
        LANGUAGES C)
add_executable(blobfish 
    bit_twiddling.c
    move.c
    move_generation.c
    move_list.c
    move_make_unmake.c
    chess.c)
