# Blobfish2
Blobfish2 is a chess engine.

The spiritual sequel to Blobfish, written in pure (not very portable, but trying) C.

This engine is accompanied by a tutorial book (WIP).

# Book

WIP

## Building
Ease of building Blobfish2 and portability is a goal. Portability isn't always trivial when writing a chess engine
due to the usage of architecture-specific bit-twiddling functions such as "index of LSB". Using SIMD only complicates the matter further.

### Generating Headers


### CMake
CMake

Building in release mode: `cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release`

## Programming Pitfalls
- In haste, there are a lot of infinite loops in parsing.
- Issues with unsigned subtraction

## UCI Compatibility
- Right now, the engine implements the bare minimum for compatibility with UCI GUIs.
- Tested with `cutechess`

## Testing

### Correctness

### Strength Estimation
Tournament requirements: blobfish2, stockfish, cutechess-cli commands available in $PATH.

### Puzzles
Lichess puzzle requirements: puzzle database downloaded and extracted from https://database.lichess.org/#puzzles
- Any checkmate move wins the puzzle

### Performance

