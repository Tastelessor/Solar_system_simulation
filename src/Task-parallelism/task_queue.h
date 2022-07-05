#include "types.h"
#include "stdbool.h"

void create_queue(task_queue*);
bool is_empty(task_queue*);
void push(task_queue*, Task);
Task pop(task_queue*);
void clear(task_queue*);