#include "chess.h"

MoveList move_list_create() {
    MoveList list;
    list.count = 0;
    return list;
}

Move move_list_get(MoveList *list, i32 index) {
    return list->_stack_data[index];
}

void move_list_push(MoveList *list, Move mv) {
    list->_stack_data[list->count++] = mv;
}
 
void move_list_destroy(MoveList *list) {
    // not needed yet
}