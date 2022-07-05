#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <stdbool.h>
#include "simulation_configuration.h"
#include "simulation_support.h"
#include "Task-parallelism/worker.h"

// The bodies that are involved in the simulation
struct body_struct *bodies;
struct body_history *bodies_history;

char *filename;
worker process;
int file_output_num = 0, history_index = 0;
int number_active_bodies = 0, num_asteroids = 0, num_comets = 0; // count number of corresponding bodies
int stride; // Define how many iterations a process should run
int max_body_size; // The maximum size of body
int *gather_count; // Number of elements a process would pass in MPI_Gatherv()
int *gather_displacement; // Displacement of a process in MPI_Gatherv()
int size, rank; // Total number of processes and rank of the this process
int start, end; // start index and end index for iterations
int collisions_asteroids = 0; // Total number of collisions with asteroids
int collisions_comets = 0;  // Total number of collisions with comets

struct timeval start_time;
char display_buffer[1000];

struct simulation_configuration_struct configuration;

MPI_Datatype bodies_type; // Self-defined MPI Datatype
MPI_Comm comm = MPI_COMM_WORLD;
MPI_Request request;

static void initialise_function(int argc, char *argv[]);

static void initialise_bodies(struct simulation_configuration_struct *);

static void step(double, int, struct simulation_configuration_struct *, char *);

static char *parseSecondsToDays(long int, char *);

static void update_thread();

static void update_locations(double);

static void update_body_acceleration(int);

static void handle_collision(int, int);

static void compute_velocity(double);

static void initialise_bodies();

static void store_history(char *);

static void dump_history_to_file(char *);

static double getElapsedTime(struct timeval);

static void check_collisions();

static void end_simulate();

static void comet_invade();

static void gather_broadcast();

static void broadcast();

static void print_frequently();

/*
* Using the framework, the simulation process can be done in 20 lines of code
*/
int main(int argc, char *argv[]) {
    void **args; // argument list for passing arguments
    void **empty; // empty argument list
    args = malloc(sizeof(void *) * 1);
    args[0] = &process;

    initialize_worker(&process, comm, &initialise_function, argc, argv);

    for (int i = 0; i < configuration.num_timesteps; i++) {
        load_task(&process, &update_thread, args, 1);
        load_task(&process, &compute_velocity, empty, 0);
        load_task(&process, &update_locations, empty, 0);
        load_task(&process, &gather_broadcast, empty, 0);
        if (process.id == 0) {
            load_task(&process, &comet_invade, empty, 0);
        }
        load_task(&process, &check_collisions, empty, 0);
        load_task(&process, &broadcast, empty, 0);
        if (process.id == 0)
            load_task(&process, &print_frequently, empty, 0);
    }
    load_task(&process, &end_simulate, empty, 0);

    work(&process);
//    printf("Finish simulation\n");
}

/*
 * Update the start and end index for a process
 */
static void update_thread(worker *man) {
    // If there is only one process, then it does all the work
    if (process.population <= 1) {
        start = 0;
        end = number_active_bodies;
        return;
    }

    // Update stride, which is the number of iterations that a process should work for
    stride = number_active_bodies / process.population;

    start = process.id * stride;
    end = start + stride;
    // In case that the number of bodies can not be divided by the number of processes
    if (process.id == process.population - 1)
        end = number_active_bodies;

    // Update count and displacement for MPI_Gatherv()
    for (int i = 0; i < process.population; i++) {
        gather_count[i] = stride;
        gather_displacement[i] = i * stride;
    }
    gather_count[process.population - 1] = number_active_bodies - (process.population - 1) * stride;
}


/*
* Generate a comet randomly, the comet's name is initialized without sequence number
* The sequence number will be attached to the end of its name after it is generated
* In this way, number of passed parameters is reduced
*/
static void comet_invade() {
    // Check whether a comet will invade at this timestamp, if it is, initialise it
    if (random_comet(&bodies[number_active_bodies])) {
        char buffer[5];
        sprintf(buffer, " %d", num_comets++);
        strcpy(bodies[number_active_bodies].name, "COMET");
        strcat(bodies[number_active_bodies].name, buffer);
        tostring(&bodies[number_active_bodies]);

        bodies_history[number_active_bodies].history_x = (double *) calloc(MAX_HISTORY_SIZE, sizeof(double));
        bodies_history[number_active_bodies].history_y = (double *) calloc(MAX_HISTORY_SIZE, sizeof(double));
        bodies_history[number_active_bodies++].history_z = (double *) calloc(MAX_HISTORY_SIZE, sizeof(double));
    }
}

/*
 * Output history data frequently
 */
static void print_frequently() {
    if (process.loop_index % configuration.output_frequency == 0) store_history(filename);
    if (process.loop_index > 0 && process.loop_index % configuration.display_progess_frequency == 0) {
        printf("Timestep: %d, model time is %s, current runtime is %.2f seconds, %d bodies studied\n",
               process.loop_index,
               parseSecondsToDays((long int) process.loop_index * (long int) configuration.dt, display_buffer),
               getElapsedTime(start_time), number_active_bodies);
        // Print number of collisions with asteroids and comets for every sun, planet and moon
        for (int j = 0; j < number_active_bodies; j++) {
            if (bodies[j].type < 3) {
                printf("For %s, number of collisions with asteroids: %d, with comets: %d\n",
                       bodies[j].name,
                       bodies[j].collided_asteroids,
                       bodies[j].collided_comets);
            }
        }
    }
    process.loop_index++;
}

/*
* Output statistical data and store history data after simulation
*/
static void end_simulate() {
    if (process.id == 0) {
        if (history_index > 0) dump_history_to_file(filename);
        // Reports the total number of collisions
        printf("Timestep: %d, model time is %s, current runtime is %.2f seconds\n",
               configuration.num_timesteps,
               parseSecondsToDays((long int) configuration.num_timesteps * (long int) configuration.dt, display_buffer),
               getElapsedTime(start_time));
        for (int j = 0; j < number_active_bodies; j++) {
            if (bodies[j].type < 3) {
                printf("For %s, number of collisions with asteroids: %d, with comets: %d\n",
                       bodies[j].name,
                       bodies[j].collided_asteroids,
                       bodies[j].collided_comets);
                /*
                 * Sum up the number of collisions
                 */
                collisions_asteroids += bodies[j].collided_asteroids;
                collisions_comets += bodies[j].collided_comets;
            }
        }
        printf("------------------------------------------------\n");
        printf("Model completed after %d timesteps\nTotal model time: %s\nTotal runtime: %.2f seconds\n",
               configuration.num_timesteps,
               parseSecondsToDays((long int) configuration.num_timesteps * (long int) configuration.dt, display_buffer),
               getElapsedTime(start_time));
        // Print the statistical results of collisions
        printf("Total sum of collisions with the sun, planets and moons:\n"
               "asteroids: %d\t comets:%d\n", collisions_asteroids, collisions_comets);
    }
    MPI_Type_free(&bodies_type);
    MPI_Finalize();
}

/*
 * Collect data from all processes
 * First, all processes send their local updated data to process 0
 * Then, Process 0 broadcast the updated results to all processes
 */
static void gather_broadcast() {
    if (process.id == 0)
        MPI_Gatherv(MPI_IN_PLACE, end - start, bodies_type, bodies, gather_count, gather_displacement, bodies_type, 0,
                    comm);
    else
        MPI_Gatherv(&bodies[start], end - start, bodies_type, NULL, NULL, NULL, bodies_type, 0, comm);

    MPI_Bcast(&bodies[0], number_active_bodies, bodies_type, 0, comm);
}

/*
 * Broadcast synchronized data to all processes
 */
static void broadcast() {
    // Broadcast the total number of bodies for now to all processes
    MPI_Bcast(&number_active_bodies, 1, MPI_INT, 0, comm);
    // Broadcast the updated results to all processes after checking collisions
    MPI_Bcast(&bodies[0], number_active_bodies, bodies_type, 0, comm);
}

/*
* Will check for collisions between all bodies. These are handled differently depending upon whether the body is a
* planet, moon, asteroid, or the sun.
*/
static void check_collisions() {
    /*
     * pair_code = i * max_body_size + j
     * In this way, i and j can be passed at the same time with only one variable
     * To decode, j = pair_code % max_body_size, i = (pair_code - j) / max_body_size
     */
    int pair_code;
    int length = 10;
    int index = 0;

    /*
     *  Requests of MPI_Isend for all processes except process 0
     *  The requests are defined as a dynamic array in case that many collisions are detected by one process
     *  The size of the request array will be initialised to 10, and it will double every time the array size
     *  is not enough
     */
    MPI_Request *requests_collision;
    requests_collision = (MPI_Request *) malloc(sizeof(MPI_Request) * length);

    int reverse_start = number_active_bodies - end;
    int reverse_end = number_active_bodies - start;

    for (int i = reverse_start; i < reverse_end; i++) {
        // Now check for bodies i+1, so don't check own body but all beyond it in the bodies array
        // Don't check any earlier as we have symmetry here so would mean duplicate checks and updates
        for (int j = i + 1; j < number_active_bodies; j++) {
            if (bodies[i].active && bodies[j].active && !((bodies[i].type == MOON && bodies[j].type == PLANET) ||
                                                          (bodies[j].type == MOON && bodies[i].type == PLANET))) {
                if (checkForCollision(&bodies[i], &bodies[j])) {
                    if (process.id == 0) {
                        handle_collision(i, j);
                    } else {
                        if (index == length) {
                            length *= 2;
                            requests_collision = (MPI_Request *) realloc(requests_collision,
                                                                         sizeof(MPI_Request) * length);
                        }
                        pair_code = i * max_body_size + j;
                        MPI_Isend(&pair_code, 1, MPI_INT, 0, process.id, comm, &requests_collision[index++]);
                    }
                }
            }
        }
    }

    if (process.id == 0) {
        int temp_pair_code;
        MPI_Status status;

        /*
         * This loop handles messages from other processes consequently
         * Every time it receives an end message (a message with tag 0), the i increases 1
         * It ends if messages from all processes are handled
         * If the message tag is not 0, then it decodes the message and handle the collision
         */
        for (int i = 1; i < process.population;) {
            MPI_Recv(&temp_pair_code, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &status);
            if (status.MPI_TAG == 0)
                i++;
            else {
                int temp = temp_pair_code % max_body_size;
                handle_collision((temp_pair_code - temp) / max_body_size, temp);
            }
        }
    } else {
        // Complete non-blocking sends
        MPI_Waitall(index, requests_collision, MPI_STATUS_IGNORE);
        // Complete sending collision report
        MPI_Ssend(&index, 1, MPI_INT, 0, 0, comm);
    }
}

/*
 * This function is basically the same as the provided code, it is used for handling collisions between two bodies
 * However, this implementation includes collisions with moon. In addition, there is a one in ten chance that two
 * collided asteroids will split into four asteroids.
 */
static void handle_collision(int i, int j) {
    printf("Collision between %s and %s, their state: %d and %d\n", bodies[i].name, bodies[j].name,
           bodies[i].active, bodies[j].active);
    if (bodies[i].type == ASTEROID && bodies[j].type == ASTEROID) {
        /*
         * Check if the two asteroids shall split into four asteroids
         * Collision behaviour is encapsulated in the function handle_asteroid_asteroid_bodies()
         */
        if (handle_asteroid_asteroid_collision(&bodies[i], &bodies[j])) {
            char buffer[5];
            for (int k = number_active_bodies; k < 4 + number_active_bodies; k++) {
                sprintf(buffer, "%d", num_asteroids++);
                strcpy(bodies[k].name, "ASTEROIDS");
                strcat(bodies[k].name, buffer);
                bodies_history[k].history_x = (double *) calloc(MAX_HISTORY_SIZE, sizeof(double));
                bodies_history[k].history_y = (double *) calloc(MAX_HISTORY_SIZE, sizeof(double));
                bodies_history[k].history_z = (double *) calloc(MAX_HISTORY_SIZE, sizeof(double));
            }
            split_asteroid(&bodies[i], &bodies[number_active_bodies++], true);
            split_asteroid(&bodies[i], &bodies[number_active_bodies++], false);
            split_asteroid(&bodies[j], &bodies[number_active_bodies++], true);
            split_asteroid(&bodies[j], &bodies[number_active_bodies++], false);
        }
    } else if ((bodies[i].type == ASTEROID || bodies[i].type == COMET) &&
               (bodies[j].type == PLANET || bodies[j].type == SUN || bodies[j].type == MOON)) {
        handle_planet_asteroid_collision(&bodies[j], &bodies[i]);
    } else if ((bodies[i].type == PLANET || bodies[i].type == SUN || bodies[i].type == MOON) &&
               (bodies[j].type == ASTEROID || bodies[i].type == COMET)) {
        handle_planet_asteroid_collision(&bodies[i], &bodies[j]);
    } else if (bodies[i].type == COMET && bodies[j].type == COMET) {
        handle_comet_comet_collision(&bodies[i], &bodies[j]);
    } else if (bodies[i].type == ASTEROID && bodies[j].type == COMET) {
        handle_asteroid_comet_collision(&bodies[i], &bodies[j]);
    } else if (bodies[i].type == COMET && bodies[j].type == ASTEROID) {
        handle_asteroid_comet_collision(&bodies[j], &bodies[i]);
    }
}

/*
* Computes the velocity of all bodies in the simulation based upon their gravitational interactions with all other bodies
*/
static void compute_velocity() {
    for (int i = start; i < end; i++) {
        if (bodies[i].active) {
            update_body_acceleration(i);
            bodies[i].velocity_x += bodies[i].acceleration_x * configuration.dt;
            bodies[i].velocity_y += bodies[i].acceleration_y * configuration.dt;
            bodies[i].velocity_z += bodies[i].acceleration_z * configuration.dt;
        }
    }
}

/*
* Based on the velocity of each body will update its location
*/
static void update_locations() {
    for (int i = start; i < end; i++) {
        if (bodies[i].active) {
            bodies[i].x += bodies[i].velocity_x * configuration.dt;
            bodies[i].y += bodies[i].velocity_y * configuration.dt;
            bodies[i].z += bodies[i].velocity_z * configuration.dt;
        }
    }
}

/*
* For a body will loop through every other active body and calculate the gravitational force interaction between them
*/
static void update_body_acceleration(int index) {
    struct body_struct *my_body = &bodies[index];
    my_body->acceleration_x = 0;
    my_body->acceleration_y = 0;
    my_body->acceleration_z = 0;
    for (int i = 0; i < number_active_bodies; i++) {
        if (i != index && bodies[i].active) {
            calculate_two_body_acceleration(my_body, &bodies[i]);
        }
    }
}

/*
* Will store the current location of each body in its history. If that history becomes full then it will be written out (appended) to
* the output file and history counter reset
*/
static void store_history(char *filename) {
    for (int i = 0; i < number_active_bodies; i++) {
        bodies_history[i].history_x[history_index] = bodies[i].x;
        bodies_history[i].history_y[history_index] = bodies[i].y;
        bodies_history[i].history_z[history_index] = bodies[i].z;
    }
    history_index++;
    if (history_index >= MAX_HISTORY_SIZE) {
        dump_history_to_file(filename);
        history_index = 0;
    }
}

/*
* Appends all body histories to the output file whose name is provided as an argument
*/
static void dump_history_to_file(char *filename) {
    FILE *file = fopen(filename, file_output_num == 0 ? "w" : "a");
    file_output_num++;
    for (int i = 0; i < number_active_bodies; i++) {
        for (int j = 0; j < history_index; j++) {
            fprintf(file, "%s_x=%f\n", bodies[i].name, bodies_history[i].history_x[j]);
            fprintf(file, "%s_y=%f\n", bodies[i].name, bodies_history[i].history_y[j]);
            fprintf(file, "%s_z=%f\n", bodies[i].name, bodies_history[i].history_z[j]);
        }
    }
    fclose(file);
}

/*
* Based upon the configuration of the simulation this will initialise all the active bodies
* such that the simulation is ready to run
*/
static void initialise_bodies(struct simulation_configuration_struct *configuration) {
    int currentBody = 0;
    max_body_size = configuration->body_size;
    bodies = (struct body_struct *) malloc(sizeof(struct body_struct) * max_body_size);
    bodies_history = (struct body_history *) malloc(sizeof(struct body_history) * max_body_size);
    for (int i = 0; i < max_body_size; i++) {
        if (configuration->body_configurations[i].active) {
            strcpy(bodies[currentBody].name, configuration->body_configurations[i].name);
            bodies[currentBody].x = configuration->body_configurations[i].x;
            bodies[currentBody].y = configuration->body_configurations[i].y;
            bodies[currentBody].z = configuration->body_configurations[i].z;
            bodies[currentBody].mass = configuration->body_configurations[i].mass;
            bodies[currentBody].radius = configuration->body_configurations[i].radius;
            bodies[currentBody].velocity_x = configuration->body_configurations[i].velocity_x;
            bodies[currentBody].velocity_y = configuration->body_configurations[i].velocity_y;
            bodies[currentBody].velocity_z = configuration->body_configurations[i].velocity_z;
            bodies[currentBody].type = configuration->body_configurations[i].type;
            bodies[currentBody].active = true;
            bodies_history[currentBody].history_x = (double *) malloc(sizeof(double) * MAX_HISTORY_SIZE);
            bodies_history[currentBody].history_y = (double *) malloc(sizeof(double) * MAX_HISTORY_SIZE);
            bodies_history[currentBody].history_z = (double *) malloc(sizeof(double) * MAX_HISTORY_SIZE);
            int type = bodies[currentBody].type;
            if (type < 3) {
                // Initialize collision counts
                bodies[currentBody].collided_asteroids = 0;
                bodies[currentBody].collided_comets = 0;
            } else if (type == 3) {
                num_asteroids++;
            } else if (type == 4) {
                num_comets++;
            }
            currentBody++;
        }
    }
    number_active_bodies = currentBody;
}

/*
* Returns in seconds the elapsed time since the start_time argument and now
*/
static double getElapsedTime(struct timeval start_time) {
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    long int elapsedtime =
            (curr_time.tv_sec * 1000000 + curr_time.tv_usec) - (start_time.tv_sec * 1000000 + start_time.tv_usec);
    return elapsedtime / 1000000.0;
}

/*
* Parses a specific number of seconds into a pretty printed string
*/
static char *parseSecondsToDays(long int seconds, char *buffer) {
    long int remainder = seconds;
    int years = remainder / 31536000;
    remainder = remainder - (years * 31536000);
    int days = remainder / 86400;
    remainder = remainder - (days * 86400);
    int hours = remainder / 3600;
    remainder = remainder - (hours * 3600);
    int mins = remainder / 60;
    remainder = remainder - (mins * 60);
    sprintf(buffer, "%d years, %d days, %d hours, %d min and %ld secs", years, days, hours, mins, remainder);
    return buffer;
}

/*
 * Initialisation function for workers
 * This function initialise bodies and self-defined MPI data types
 */
static void initialise_function(int argc, char *argv[]) {
    if (argc < 3) {
        printf("You must provide the configuration file and output file names as arguments\n");
        exit(-1);
    }
    // Define body size if it's passed as argument
    if (argc == 4)
        configuration.body_size = atoi(argv[3]);
    else
        configuration.body_size = MAX_BODY_CONFIGS;

    /*
     * Allocate memory for the two variables to perform MPI_Gatherv()
     * The number of processes can only be known after calling MPI_Comm_size for a process
     * Hence allocate memory dynamically is helpful because it redueces memory cost
     */
    gather_count = (int *) malloc(process.population * sizeof(int));
    gather_displacement = (int *) malloc(process.population * sizeof(int));

    parseConfiguration(argv[1], &configuration);
    filename = argv[2];

    initialise_bodies(&configuration);

    if (process.id == 0) {
        printf("MPI initialized, number of threads: %d\n", process.population);
        printf("Simulation configured for %d bodies, timesteps=%d dt=%f\n", number_active_bodies,
               configuration.num_timesteps, configuration.dt);
        printf("Number of asteroids in the asteroids belt: %d\n", configuration.asteroid_belt);
        printf("Number of asteroids in the Kuiper Belt: %d\n", configuration.kuiper_belt);
        printf("------------------------------------------------\n");
    }

    gettimeofday(&start_time, NULL);

    /*
     * Initialise rand() with seed
     * If you want to make the program obtain different results every run
     * Then you can replace the seed with time(0)
     */
    srand(8759);
//    srand(time(0));

    /*
     * Commit struct bodies to MPI
     */
    int length[16] = {40, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    MPI_Aint displacement[16];
    MPI_Datatype types[16] = {MPI_CHAR, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE,
                              MPI_DOUBLE,
                              MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_C_BOOL, MPI_INT, MPI_INT, MPI_INT};

    // Define displacement and address
    struct body_struct dummy_body;
    MPI_Aint base_address;
    MPI_Get_address(&dummy_body, &base_address);
    MPI_Get_address(&dummy_body.name[0], &displacement[0]);
    MPI_Get_address(&dummy_body.x, &displacement[1]);
    MPI_Get_address(&dummy_body.y, &displacement[2]);
    MPI_Get_address(&dummy_body.z, &displacement[3]);
    MPI_Get_address(&dummy_body.mass, &displacement[4]);
    MPI_Get_address(&dummy_body.velocity_x, &displacement[5]);
    MPI_Get_address(&dummy_body.velocity_y, &displacement[6]);
    MPI_Get_address(&dummy_body.velocity_z, &displacement[7]);
    MPI_Get_address(&dummy_body.acceleration_x, &displacement[8]);
    MPI_Get_address(&dummy_body.acceleration_y, &displacement[9]);
    MPI_Get_address(&dummy_body.acceleration_z, &displacement[10]);
    MPI_Get_address(&dummy_body.radius, &displacement[11]);
    MPI_Get_address(&dummy_body.active, &displacement[12]);
    MPI_Get_address(&dummy_body.type, &displacement[13]);
    MPI_Get_address(&dummy_body.collided_asteroids, &displacement[14]);
    MPI_Get_address(&dummy_body.collided_asteroids, &displacement[15]);
    displacement[0] = MPI_Aint_diff(displacement[0], base_address);
    displacement[1] = MPI_Aint_diff(displacement[1], base_address);
    displacement[2] = MPI_Aint_diff(displacement[2], base_address);
    displacement[3] = MPI_Aint_diff(displacement[3], base_address);
    displacement[4] = MPI_Aint_diff(displacement[4], base_address);
    displacement[5] = MPI_Aint_diff(displacement[5], base_address);
    displacement[6] = MPI_Aint_diff(displacement[6], base_address);
    displacement[7] = MPI_Aint_diff(displacement[7], base_address);
    displacement[8] = MPI_Aint_diff(displacement[8], base_address);
    displacement[9] = MPI_Aint_diff(displacement[9], base_address);
    displacement[10] = MPI_Aint_diff(displacement[10], base_address);
    displacement[11] = MPI_Aint_diff(displacement[11], base_address);
    displacement[12] = MPI_Aint_diff(displacement[12], base_address);
    displacement[13] = MPI_Aint_diff(displacement[13], base_address);
    displacement[14] = MPI_Aint_diff(displacement[14], base_address);
    displacement[15] = MPI_Aint_diff(displacement[15], base_address);

    MPI_Type_create_struct(16, length, displacement, types, &bodies_type);
    MPI_Type_commit(&bodies_type);
}
