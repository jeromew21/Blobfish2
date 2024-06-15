#include "chess.h"
// Consider globally moving to signed ints.

u32 move_get_metadata(Move mv) { return (mv >> 12) & 0xf; }

u64 move_get_dest(Move mv) {
  const u32 dest = mv & 0x3f;
  return (u64)1 << dest;
}

u64 move_get_src(Move mv) {
  const u32 src = (mv >> 6) & 0x3f;
  return (u64)1 << src;
}

u32 move_get_dest_u32(Move mv) { return mv & 0x3f; }

u32 move_get_src_u32(Move mv) { return (mv >> 6) & 0x3f; }

Move move_create(u32 src, u32 dest, u32 flags) {
  // 0xf  = 0b00001111
  // 0x3f = 0b00111111
  return ((flags & 0xf) << 12) | ((src & 0x3f) << 6) | (dest & 0x3f);
}

/*
 * Buffer must be at least 6 characters long to account for null terminator.
 */
void move_to_string(Move mv, char *buf) {
    static char row_names[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    static char col_names[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    u32 src = move_get_src_u32(mv);
    u32 dest = move_get_dest_u32(mv);
    char special[4];
    special[0] = '\0';
    u32 md = move_get_metadata(mv);
    if (md == kQueenPromotionMove || md == kQueenCapturePromotionMove) {
        sprintf(special, "%s", "q");
    } else if (md == kKnightPromotionMove || md == kKnightCapturePromotionMove) {
        sprintf(special, "%s", "n");
    } else if (md == kBishopPromotionMove || md == kBishopCapturePromotionMove) {
        sprintf(special, "%s", "b");
    } else if (md == kRookPromotionMove || md == kRookCapturePromotionMove) {
        sprintf(special, "%s", "r");
    }
    sprintf(buf, "%c%c%c%c%s", col_names[src % 8], row_names[src / 8],
            col_names[dest % 8], row_names[dest / 8], special);
}
