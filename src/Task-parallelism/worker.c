#include "worker.h"

/*
 * Initialise a worker
 * This framework is designed for MPI
 * Hence MPI initialisation is included in the initialisation function
 */
void initialize_worker(worker *man, MPI_Comm comm, void* initialize_function, int argc, char *argv[]){
    // Initialise MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &man->population);
    MPI_Comm_rank(comm, &man->id);
    man->loop_index = 0;
    create_queue(&man->task_q);

    // Invoke initializing function
    void (*function)(int, char**) = initialize_function;
    (*function)(argc, argv);
}

/*
 * Update start and end indices for a worker
 * Note this function is not used due to the use of MPI_Gatherv()
 * Another function that updates indices and variables related to gather is created in the main file
 * It's called update_threads()
 */
void update_worker(worker* man){
    // If there is only one process, then it does all the work
    if(man->population <= 1){
        man->start_index = 0;
        man->end_index = man->problem_size;
        return;
    }

    // stride: the number of iterations that a process should work for
    int stride = man->problem_size / man->population;

    man->start_index = man->id * stride;
    man->end_index = man->start_index + stride;

    // In case that the number of bodies can not be divided by the number of processes
    if (man->id == man->population - 1)
        man->end_index = man->problem_size;
}

/*
 * Synchronize workers in a specific communicator
 */
void wait_synchronization(MPI_Comm comm){
    // Wait for synchronization
    MPI_Barrier(comm);
}

/*
 * Load a task into a queue
 */
void load_task(worker *man, void* function, void** args, int argc){
    Task task;

    task.function = function;
    task.args = args;
    task.argc = argc;

    push(&man->task_q, task);
}

/*
 * Work on tasks inside a task queue until all tasks are finished
 */
void work(worker *man){
    Task task;
    while (!is_empty(&man->task_q)){
        task = pop(&man->task_q);
        task.function(task.args);
    }
}

/*
 * Free a worker
 */
void suicide(worker *man){
    suicide(man);
    printf("Woker: %d has finished his work\n", man->id);
}