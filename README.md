# Blobfish 2
The spiritual sequel to Blobfish, written in pure (not very portable, but trying) C.
UCI-compatible (TODO).

# Book

## Testing
Tournament requirements: blobfish2, stockfish, cutechess-cli commands available in $PATH.

Lichess puzzle requirements: puzzle database downloaded and extracted from https://database.lichess.org/#puzzles

## Building
Ease of building Blobfish2 and portability is a goal.
- There is a `CMakeLists.txt`.

## Pitfalls
- In haste, there are a lot of infinite loops in parsing.
- Issues with unsigned subtraction
