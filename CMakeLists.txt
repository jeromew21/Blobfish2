cmake_minimum_required(VERSION 3.23)
project(Blobfish
        VERSION 1.0
        LANGUAGES C)


set(blobfish_exe blobfish)
add_executable(${blobfish_exe}
        src/bit_twiddling.c
        src/fen_parse.c
        src/debug.c
        src/perft.c
        src/evaluation.c
        src/move.c
        src/move_generation.c
        src/move_list.c
        src/move_make_unmake.c
        src/search.c
        src/uci.c
        src/cli.c)

if(MSVC)
  target_compile_options(${blobfish_exe} PRIVATE /W4 /WX)
else()
  target_compile_options(${blobfish_exe} PRIVATE -Wall -Wextra -Wpedantic)
endif()

