#include <stdio.h>
#include <stdlib.h>

/*
 * Tasks are basically functions
 * A task contains: function pointer, number of arguments and argument list
 */
typedef struct Task_def{
    void* (*function)(void**);
    void** args;
    int argc;
}Task;

/*
 * Node of task queue
 */
typedef struct Node_def{
    Task task;
    struct Node_def* next;
}Node;

/*
 * Definition of task queue
 */
typedef struct task_queue_def{
    int size;
    struct Node_def* head;
    struct Node_def* tail;
}task_queue;

/*
 * Definition of worker
 */
typedef struct worker_def{
    int id;
    int population;
    int problem_size;
    int loop_index;
    int start_index;
    int end_index;
    task_queue task_q;
}worker;