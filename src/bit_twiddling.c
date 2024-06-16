#include "chess.h"

// note that these builtins aren't portable.
// there might be a different builtin somewhere
// there is a way to drop into assembly but not sure if that's a good idea

/**
 * Get index of first set bit from least significant direction.
 * https://www.chessprogramming.org/BitScan#x86
 */
u32 bitscan_forward(u64 bitset) { return __builtin_ffsll(bitset) - 1; }

/**
 * Get index of first set bit but from more significant side.
 */
u32 bitscan_reverse(u64 bitset) { return 63 - __builtin_clzll(bitset); }

/**
 * Get number of bit set to 1 in the bitset.
 */
i32 pop_count(u64 bitset) { return __builtin_popcountll(bitset); }

/**
 * For magics generation
 *
 * Return the i-th permutation of mask with pop_count members
 * Original method by Tord Romstad
 */
u64 permute_mask(u64 mask, i32 index, i32 pop_count) {
  i32 i, j;
  u64 result = 0;
  for (i = 0; i < pop_count; i++) {
    j = (i32)bitscan_forward(mask);
    mask &= ~(((u64)1) << j);

    if (index & (1 << i))
      result |= (((u64)1) << j);
  }
  return result;
}
