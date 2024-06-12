#include "chess.h"

// note that these builtins aren't portable.
// there might be a different builtin somewhere
// there is a way to drop into assembly but idk if that's a good idea

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
u32 pop_count(u64 bitset) {
  return __builtin_popcountll(bitset);
  // https://www.chessprogramming.org/Population_Count
  // might be architecture specific
  // static const u64 m1 = 0x5555555555555555;  // binary: 0101...
  // static const u64 m2 = 0x3333333333333333;  // binary: 00110011..
  // static const u64 m4 = 0x0f0f0f0f0f0f0f0f;  // binary:  4 zeros,  4 ones ...
  // static const u64 h01 = 0x0101010101010101; // the sum of 256 to the power
  // of 0,1,2,3... u64 x = bitset; x -= (x >> 1) & m1;             // put count
  // of each 2 bits into those 2 bits x = (x & m2) + ((x >> 2) & m2); // put
  // count of each 4 bits into those 4 bits x = (x + (x >> 4)) & m4;        //
  // put count of each 8 bits into those 8 bits return (x * h01) >> 56; //
  // returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ...
}

/**
 * return the i-th permutation of mask with pop_count members
 *
 * original method by Tord Romstad with modifications
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
