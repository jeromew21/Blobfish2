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

We generate some lookup tables with a Python script, located in `scripts/`.

### CMake

Minimal example:

```
cmake -S . -B build
cmake --build build
```

Building in release mode: `cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release`

## UCI Compatibility

- Right now, the engine implements the minimum for compatibility with UCI GUIs.

UCI compliance tested with `cutechess`

## Testing

In-engine tests need to be run with repository as working directory.

### Correctness

Engine command: `test perft`

This may take a few minutes.

### Puzzles

Engine command: `test puzzles`

Lichess puzzle requirements: puzzle database downloaded and extracted from https://database.lichess.org/#puzzles into `test/`
- Any checkmate move wins the puzzle

### Performance

Engine command: `test performance`

Move generator performance is tracked by running perft(5).

### Strength Estimation

Scripts in `test/`

Requirements: `blobfish`, `stockfish`, `cutechess-cli` commands available in $PATH

## Documentation

Engine design is documented in the book (WIP).

