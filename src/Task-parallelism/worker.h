#include "task_queue.h"
#include <mpi.h>

void initialize_worker(worker*, MPI_Comm, void*, int , char *[]);
void wait_synchronization(MPI_Comm);
void load_task(worker*, void* , void** , int );
void work(worker*);
void suicide(worker*);
void update_worker(worker*);