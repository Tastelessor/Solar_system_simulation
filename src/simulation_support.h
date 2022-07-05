#ifndef SUPPORT_INCLUDE
#define SUPPORT_INCLUDE

#include <stdbool.h>

// Type of a body
enum body_type_enum {
    SUN = 0, PLANET = 1, MOON = 2, ASTEROID = 3, COMET = 4, UNKNOWN = 20
};

// Information about each body that is updated as the simulation progresses
struct body_struct {
    char name[40];
    double x, y, z;
    double mass;
    double velocity_x, velocity_y, velocity_z;
    double acceleration_x, acceleration_y, acceleration_z;
    double radius;
    bool active;
    enum body_type_enum type;
    /*
     * These two variables will be initialised only if the body is of type sun, planet or moon
     */
    int collided_asteroids; // number of collisions with asteroids
    int collided_comets; // number of collisions with comets
};

/*
 * This structure is created to store history data because it's need only in process 0
 * As a result, the three pointer variables are removed from the original structure (i.e. body_struct)
 */
struct body_history{
    double *history_x, *history_y, *history_z;
};

bool checkForCollision(struct body_struct *, struct body_struct *);

void calculate_two_body_acceleration(struct body_struct *, struct body_struct *);

void handle_planet_asteroid_collision(struct body_struct *, struct body_struct *);

bool handle_asteroid_asteroid_collision(struct body_struct *, struct body_struct *);

void split_asteroid(struct body_struct *, struct body_struct *, bool direction);

void handle_comet_comet_collision(struct body_struct *, struct body_struct *);

void handle_asteroid_comet_collision(struct body_struct *, struct body_struct *);

bool random_comet(struct body_struct *);

void tostring(struct body_struct *);

#endif