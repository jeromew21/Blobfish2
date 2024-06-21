cmake_minimum_required(VERSION 3.23)
project(Blobfish2
        VERSION 1.0
        LANGUAGES C)

set(BLOBFISH_EXE blobfish)

set(BLOBFISH_SRC_FILES src/bit_twiddling.c
        src/fen.c
        src/debug.c
        src/board.c
        src/evaluation.c
        src/move.c
        src/move_generation.c
        src/move_list.c
        src/move_make_unmake.c
        src/search.c
        src/parse.c
        src/test_puzzles.c
        src/test_perft.c
        src/test_hashing.c
        src/uci.c
        src/cli.c)

add_executable(${BLOBFISH_EXE} ${BLOBFISH_SRC_FILES})

if(MSVC)
  target_compile_options(${BLOBFISH_EXE} PRIVATE /W4 /WX)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "-g")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  target_compile_options(${BLOBFISH_EXE} PRIVATE -Wall -Wextra -Wpedantic)
endif()

