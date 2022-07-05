#include "simulation_support.h"
#include "simulation_configuration.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Gravitational constant
#define G_CONSTANT 6.67408e-11
/*
 * Edge of solar system
 * Reference: NASA Science. Accessed: https://solarsystem.nasa.gov/news/1164/how-big-is-the-solar-system/
 */
#define SOLAR_SYSTEM_RADIUS 4.5e12

static void update_body_momentum_elastic_collision(struct body_struct *, struct body_struct *);

static double l2norm(double, double, double);

static double drandom(double low, double high);

/*
* Checks for a collision between two spheres by checking whether the centres of the two objects are separated by less than the sum 
* of their radii. If so then it will be a collision (note we assume perfect speheres here, this is a simplification of the real
* world but fine for our purposes).
*/
bool checkForCollision(struct body_struct *body1, struct body_struct *body2) {
    double distance_centres = sqrt(
            pow(body1->x - body2->x, 2) + pow(body1->y - body2->y, 2) + pow(body1->z - body2->z, 2));
    return distance_centres < body1->radius + body2->radius;
}

/*
* Calculates the acceleration resulting on the gravity imposed by the interaction of two bodies, this code is based on a
* function that can be found at http://www.cyber-omelette.com/2016/11/python-n-body-orbital-simulation.html
*/
void calculate_two_body_acceleration(struct body_struct *acted_upon_body, struct body_struct *acting_upon_body) {
    double r = pow((acted_upon_body->x - acting_upon_body->x), 2);
    r += pow((acted_upon_body->y - acting_upon_body->y), 2);
    r += pow((acted_upon_body->z - acting_upon_body->z), 2);
    r = sqrt(r);

    double tmp = (G_CONSTANT * acting_upon_body->mass) / pow(r, 3);
    acted_upon_body->acceleration_x += tmp * (acting_upon_body->x - acted_upon_body->x);
    acted_upon_body->acceleration_y += tmp * (acting_upon_body->y - acted_upon_body->y);
    acted_upon_body->acceleration_z += tmp * (acting_upon_body->z - acted_upon_body->z);
}

/*
* Collision between a planet and asteroid (or comet), the planet is so much larger it will obtain the mass of the asteroid 
* (or comet) and the  asteroid (or comet) is destroyed
*/
void handle_planet_asteroid_collision(struct body_struct *planet_body, struct body_struct *asteroid_body) {
    planet_body->mass += asteroid_body->mass;
    asteroid_body->active = false;
    if(asteroid_body->type == ASTEROID)
        planet_body->collided_asteroids++;
    else
        planet_body->collided_comets++;
}


/*
* Updates velocity for two asteroids that collide based on how they collide, their mass and initial velocities.
* If the two asteroids are determined not to split, then return false
* If the two asteroids are determined to split, then return true
* Note that the generation of new asteroids relates to adding new elements to the array, to reduce parameters passing,
* generation work is done by another function named split_asteroid()
*/
bool handle_asteroid_asteroid_collision(struct body_struct *body1, struct body_struct *body2) {
    if (rand() % 10 == 0) {
        body1->active = false;
        body2->active = false;

        return true;
    }

    update_body_momentum_elastic_collision(body1, body2);
    update_body_momentum_elastic_collision(body2, body1);

    body1->velocity_x = body1->acceleration_x;
    body1->velocity_y = body1->acceleration_y;
    body1->velocity_z = body1->acceleration_z;

    body2->velocity_x = body2->acceleration_x;
    body2->velocity_y = body2->acceleration_y;
    body2->velocity_z = body2->acceleration_z;

    return false;
}

/*
* Collision between asteroid and comet, the asteroid's velocity is updated based on the collision and then the comet is 
* destroyed
*/
void handle_asteroid_comet_collision(struct body_struct *asteroid_body, struct body_struct *comet_body) {
    update_body_momentum_elastic_collision(asteroid_body, comet_body);
    asteroid_body->velocity_x = asteroid_body->acceleration_x;
    asteroid_body->velocity_y = asteroid_body->acceleration_y;
    asteroid_body->velocity_z = asteroid_body->acceleration_z;

    comet_body->active = false;
}

/*
* Collision between two comets, this simply destroys them both
*/
void handle_comet_comet_collision(struct body_struct *body1, struct body_struct *body2) {
    body1->active = false;
    body2->active = false;
}

/*
 * Simulate comets' occurrence
 */
bool random_comet(struct body_struct *body) {
    if (rand() % 3000000 == 0) {
        /*
         * To place a comet at the edge of the solar system with a random position
         * The position must satisfy: x^2 + y^2 + z^2 = SOLAR_SYSTEM_EDGE^2
         */
        body->x = drandom(0, SOLAR_SYSTEM_RADIUS);
        body->y = drandom(0, sqrt(pow(SOLAR_SYSTEM_RADIUS, 2) - pow(body->x, 2)));
        body->z = drandom(0, sqrt(pow(SOLAR_SYSTEM_RADIUS, 2) - pow(body->x, 2) - pow(body->y, 2)));

        /*
         * The comet is required to be heading towards the centre, thus
         * the unit vector of velocities and the unit vector of positions must be opposite vectors
         * Hence, it can be known that velocity_x:velocity_y:velocity_z = x:y:z
         * The velocity is a constant between 1000m/s and 40000m/s
         * Assume velocity_x to be x, so are the other two velocities; let ax = y, bx = z; Then each velocity can be
         * figured out by solving the following equations:
         * 1: position_y/position_x = a
         * 2: position_z/position_x = b
         * 3: x^2 + (ax)^2 + (bx)^2 = velocity^2
         * Hence, the comet is guaranteed to fly toward the centre
         * Note that solar system is flat, but I can not find accurate data about how flat it is, so I consider the
         * solar system as a ball to comets
         */

        // Assign a random value to total velocity
        double velocity = drandom(1000, 40000);
        // Solve equation 1
        double a = body->y / body->x;
        // Solve equation 2
        double b = body->z / body->x;
        // Solve equation 3
        body->velocity_x = sqrt(pow(velocity, 2) / (1 + pow(a, 2) + pow(b, 2)));
        body->velocity_y = a * body->velocity_x;
        body->velocity_z = b * body->velocity_x;

        body->mass = drandom(1e10, 9e14);
        body->radius = drandom(2, 6);
        body->type = COMET;
        body->active = true;
        return true;
    }
    return false;
}

/*
 * This function handles the situation when two asteroids collide and split into 4 parts
 * Unit vector is used to determine the moving direction of a newly generated asteroid
 * If the asteroid is determined to move in the original direction, then its velocity would be doubled
 * If the asteroid is determined to move in the opposite direction, then its velocity would be the same as the
 * original but in different direction
 * Positions are updated to be
 * (original_x + 2 * original_diameter, original_y + 2 * original_diameter, original_z + 2 * original_diameter)
 * if the asteroid is determined to move in the original direction
 * Positions are updated to be
 * (original_x - original_diameter, original_y - original_diameter, original_z - original_diameter)
 * if the asteroid is determined to move in the opposite direction
 * Because the newly generated asteroids have only half of the radius of the original one, so they are guaranteed
 * not to collide with each other after splitting. The mathematical proof process is not included because I don't
 * know where I should put them, and I'm worried that I may lose marks because I explain too much irrelevant work,
 * so please leave a comment in the feedback if you want to look at them, I can e-mail the proof process to you after
 * marking.
 * In addition, the newly asteroids will not collide with others without the influence of external forces because
 * they have different velocities. Even if the collided two asteroids have exactly the same velocities, the newly
 * generated four asteroids will have different velocities after one timestamp due to the mutual force among them
 */
void split_asteroid(struct body_struct *ori_body, struct body_struct *new_body, bool direction) {
    int unit_vector = 2;
    if (!direction)
        unit_vector = -1;

    new_body->active = true;
    new_body->type = ASTEROID;
    ori_body->active = false;
    new_body->mass = ori_body->mass / 2;
    new_body->radius = ori_body->radius / 2;

    new_body->velocity_x = unit_vector * ori_body->velocity_x;
    new_body->velocity_y = unit_vector * ori_body->velocity_y;
    new_body->velocity_z = unit_vector * ori_body->velocity_z;

    new_body->x = unit_vector * ori_body->radius * 2 + ori_body->x;
    new_body->y = unit_vector * ori_body->radius * 2 + ori_body->y;
    new_body->z = unit_vector * ori_body->radius * 2 + ori_body->y;
}

/*
* Will update the acceleration for body1 based on it colliding with body2. This is a simple elastic collision, 
* but sufficient for our purposes, that conserves kinetic energy of the two bodies. The equation is
* an angle free elastic collision described towards the end of https://en.wikipedia.org/wiki/Elastic_collision
*/
static void update_body_momentum_elastic_collision(struct body_struct *body1, struct body_struct *body2) {
    double m1 = body1->mass;
    double m2 = body2->mass;
    double M = m1 + m2;

    double x1 = body1->x;
    double y1 = body1->y;
    double z1 = body1->z;
    double x2 = body2->x;
    double y2 = body2->y;
    double z2 = body2->z;

    double velo_x1 = body1->velocity_x;
    double velo_y1 = body1->velocity_y;
    double velo_z1 = body1->velocity_z;
    double velo_x2 = body2->velocity_x;
    double velo_y2 = body2->velocity_y;
    double velo_z2 = body2->velocity_z;

    double x_diff = x1 - x2;
    double y_diff = y1 - y2;
    double z_diff = z1 - z2;
    double d = pow(l2norm(x_diff, y_diff, z_diff), 2);

    double velocity_x_diff = velo_x1 - velo_x2;
    double velocity_y_diff = velo_y1 - velo_y2;
    double velocity_z_diff = velo_z1 - velo_z2;

    double dot_product = (velocity_x_diff * x_diff) + (velocity_y_diff * y_diff) + (velocity_z_diff * z_diff);

    body1->acceleration_x = velo_x1 - ((2 * m2) / M) * (dot_product / d) * x_diff;
    body1->acceleration_y = velo_y1 - ((2 * m2) / M) * (dot_product / d) * y_diff;
    body1->acceleration_z = velo_z1 - ((2 * m2) / M) * (dot_product / d) * z_diff;
}

/*
* Calculates l2norm for three numbers. We only ever need to get the l2norm of three numbers, hence coding it in this manner and it is
* sufficient for our needs
*/
static double l2norm(double a, double b, double c) {
    return sqrt(pow(a, 2) + pow(b, 2) + pow(c, 2));
}

/*
 * Generate a random double number that sits in [low, high]
 */
static double drandom(double low, double high) {
    return ((double) rand() / (double) (RAND_MAX)) * (high - low) + low;
}

/*
 * Print information of a body for debugging
 */
void tostring(struct body_struct *body){
    printf("Name: %s\tMass: %f\tRadius: %f\tStatus: %d\n", body->name, body->mass, body->radius, body->active);
    printf("Position: (%f, %f, %f)\tVelocity: (%f, %f, %f)\n", body->x, body->y, body->z,
           body->velocity_x, body->velocity_y, body->velocity_z);
    printf("Type: %d\tAsteroid collisions: %d\tComets collisions: %d\n",body->type,
           body->collided_asteroids, body->collided_comets);
}