#include "chess.h"

u32 move_get_metadata(Move mv) {
    return (mv >> 12) & 0xf;
}

u64 move_get_dest(Move mv) {
    const u32 dest = mv & 0x3f;
    return (u64) 1 << dest;
}

u64 move_get_src(Move mv) {
    const u32 src = (mv >> 6) & 0x3f;
    return (u64) 1 << src;
}

u32 move_get_dest_u32(Move mv) {
    return mv & 0x3f;
}

u32 move_get_src_u32(Move mv) {
    return (mv >> 6) & 0x3f;
}

Move move_create(u32 src, u32 dest, u32 flags) {
    // 0xf  = 0b00001111
    // 0x3f = 0b00111111
    return ((flags & 0xf) << 12) | ((src & 0x3f) << 6) | (dest & 0x3f);
}
