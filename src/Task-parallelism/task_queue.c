#include "task_queue.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Initialise a task queue
 */
void create_queue(task_queue *temp) {
    temp->size = 0;
    temp->head = NULL;
    temp->tail = NULL;
}

/*
 * Check if a task queue is empty
 */
bool is_empty(task_queue *tq) {
    if (tq->size <= 0)
        return true;
    return false;
}

/*
 * Push a task into the queue
 */
void push(task_queue *tq, Task t) {
    Node *temp = (Node *) malloc(sizeof(Node));
    temp->task = t;
    temp->next = NULL;

    if (tq->tail == NULL) {
        tq->head = temp;
        tq->tail = temp;
    } else {
        tq->tail->next = temp;
        tq->tail = temp;
    }
    tq->size++;
}

/*
 * Pop a task out of the queue
 */
Task pop(task_queue *tq) {
    Task task;
    if (tq->head == NULL)
        return task;

    Node *temp = tq->head;
    task = tq->head->task;

    if (tq->head->next) {
        tq->head = temp->next;
    } else {
        tq->head = NULL;
    }
    tq->size--;
    free(temp);
    return task;
}

/*
 * Clear all tasks in a queue
 */
void clear(task_queue *tq) {
    Node *cur = tq->head;
    Node *temp;

    while (cur != NULL) {
        temp = cur;
        cur = cur->next;
        free(temp);
    }

    tq->head = NULL;
    tq->tail = NULL;
}


