#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include "simulation_configuration.h"

#define MAX_LINE_LENGTH 128

static int getIntValue(char *);

static double getDoubleValue(char *);

static bool hasValue(char *);

static int getEntityNumber(char *);

static void initialiseSimulationConfiguration(struct simulation_configuration_struct *);

static void auto_generate_position(struct body_config_struct *, double, double, double);

static double drandom(double low, double high);

static enum body_type_enum getBodyType(char *);

/*
 * This function will generate a certain number of asteroids between Mars and Jupiter
 * Note that the number of asteroids can be specified in the configuration files
 * If the number of asteroids is not specified, then it will be 0 by default
 * The generated asteroids satisfy following requirements:
 * 1: radius > 1km
 * 2: Ceres, Vesta, Pallas and Hygiea are the 4 largest asteroids
 * 3: Ceres, Vesta, Pallas and Hygiea contain half of the mass of the asteroids in the belt
 * Ceres: Mass: 9.3835e20 kg, Mean radius: 469730 m [1]
 * Vesta: Mass: 2.59076e20 kg, Mean radius: 262700 m [2]
 * Pallas: Mass: 2.04e20 kg, Mean radius: 259500 m [3]
 * Hygiea: Mass: 87.4e18 kg, Mean radius: 433000 m [4]
 * Reference:
 * [1] "Asteroid Ceres P_constants (PcK) SPICE kernel file". NASA Navigation and Ancillary Information Facility. Available: https://naif.jpl.nasa.gov/pub/naif/DAWN/kernels/pck/dawn_ceres_v06.tpc
 * [2] Russell, C. T.; et al. (2012). "Dawn at Vesta: Testing the Protoplanetary Paradigm" (PDF). Science. 336 (6082): 684–686.
 * [3] Marsset, M, Brož, M, Vernazza, P, et al. (2020). "The violent collisional history of aqueously evolved (2) Pallas". Nature Astronomy. 4 (6): 569–576.
 * [4] P. Vernazza et al. (2021) VLT/SPHERE imaging survey of the largest main-belt asteroids: Final results and synthesis. Astronomy & Astrophysics 54, A56
 */

void auto_generate_asteroid_belt(struct simulation_configuration_struct *simulation_configuration, int index) {
    double max_distance_to_sun = 740520e6 - 71492000; // avoid colliding with Jupiter after initialization
    double min_distance_to_sun = 206620e6 - 3389500; // avoid colliding with Mars after initialization
    double remain_mass =
            9.3835e20 + 2.59076e20 + 2.04e20 + 87.4e18; // the remaining mass that can be assigned to other asteroids
    double average_mass = remain_mass /
                          simulation_configuration->asteroid_belt; // average mass, to assign mass to asteroids according to its radius
    double max_radius = 259500; // smaller that Pallas
    double min_radius = 1000;
    double average_radius = (max_radius + min_radius) / 2;

    double start_x = -max_distance_to_sun;
    double increment = max_distance_to_sun * 2 / simulation_configuration->asteroid_belt;

    strcpy(simulation_configuration->body_configurations[index].name, "CERES");
    simulation_configuration->body_configurations[index].radius = 469730;
    simulation_configuration->body_configurations[index].mass = 9.3835e20;
    auto_generate_position(&simulation_configuration->body_configurations[index], start_x, min_distance_to_sun,
                           max_distance_to_sun);
    start_x += increment;
    simulation_configuration->body_configurations[index].velocity_x = 0;
    simulation_configuration->body_configurations[index].velocity_y = 45000;
    simulation_configuration->body_configurations[index].velocity_z = 0;
    simulation_configuration->body_configurations[index].active = true;
    simulation_configuration->body_configurations[index++].type = ASTEROID;

    strcpy(simulation_configuration->body_configurations[index].name, "VESTA");
    simulation_configuration->body_configurations[index].radius = 262700;
    simulation_configuration->body_configurations[index].mass = 2.59076e20;
    auto_generate_position(&simulation_configuration->body_configurations[index], start_x, min_distance_to_sun,
                           max_distance_to_sun);
    start_x += increment;
    simulation_configuration->body_configurations[index].velocity_x = 0;
    simulation_configuration->body_configurations[index].velocity_y = 45000;
    simulation_configuration->body_configurations[index].velocity_z = 0;
    simulation_configuration->body_configurations[index].active = true;
    simulation_configuration->body_configurations[index++].type = ASTEROID;

    strcpy(simulation_configuration->body_configurations[index].name, "PALLAS");
    simulation_configuration->body_configurations[index].radius = 259500;
    simulation_configuration->body_configurations[index].mass = 2.04e20;
    auto_generate_position(&simulation_configuration->body_configurations[index], start_x, min_distance_to_sun,
                           max_distance_to_sun);
    start_x += increment;
    simulation_configuration->body_configurations[index].velocity_x = 0;
    simulation_configuration->body_configurations[index].velocity_y = 45000;
    simulation_configuration->body_configurations[index].velocity_z = 0;
    simulation_configuration->body_configurations[index].active = true;
    simulation_configuration->body_configurations[index++].type = ASTEROID;

    strcpy(simulation_configuration->body_configurations[index].name, "HYGIEA");
    simulation_configuration->body_configurations[index].radius = 433000;
    simulation_configuration->body_configurations[index].mass = 87.4e18;
    auto_generate_position(&simulation_configuration->body_configurations[index], start_x, min_distance_to_sun,
                           max_distance_to_sun);
    start_x += increment;
    simulation_configuration->body_configurations[index].velocity_x = 0;
    simulation_configuration->body_configurations[index].velocity_y = 45000;
    simulation_configuration->body_configurations[index].velocity_z = 0;
    simulation_configuration->body_configurations[index].active = true;
    simulation_configuration->body_configurations[index++].type = ASTEROID;

    for (int i = 0; i < simulation_configuration->asteroid_belt - 4; i++, index++) {
        char buffer[10];
        sprintf(buffer, "%d", index);
        strcpy(simulation_configuration->body_configurations[index].name, "BODY");
        strcat(simulation_configuration->body_configurations[index].name, buffer);
        simulation_configuration->body_configurations[index].radius = drandom(min_radius, max_radius);
        simulation_configuration->body_configurations[index].mass =
                average_mass * simulation_configuration->body_configurations[index].radius / average_radius;
        auto_generate_position(&simulation_configuration->body_configurations[index], start_x, min_distance_to_sun,
                               max_distance_to_sun);
        start_x += increment;
        simulation_configuration->body_configurations[index].velocity_x = 0;
        simulation_configuration->body_configurations[index].velocity_y = 45000;
        simulation_configuration->body_configurations[index].velocity_z = 0;
        simulation_configuration->body_configurations[index].active = true;
        simulation_configuration->body_configurations[index].type = ASTEROID;
    }
}

/*
 * This function will generate a certain number of asteroids in the Kuiper belt beyond the Neptune
 * Note that the number of asteroids can be specified in the configuration files
 * If the number of asteroids is not specified, then it will be 0 by default
 * The generated asteroids satisfy the requirement that radius > 100 km
 * The outer edge of the Kuiper belt is about 7.1 billion km away from the sun [1]
 * The mean density of asteroids is about 2g/cm³, hence the density of the generated asteroids will be 1g/cm³ ~ 10g/cm³
 * Reference:
 * [1] Delsemme, A. H. and Kavelaars, . J.J. (2022, January 31). Kuiper belt. Encyclopedia Britannica. https://www.britannica.com/place/Kuiper-belt
 * [2] Krasinsky, G. A.; Pitjeva, E. V.; Vasilyev, M. V.; Yagudina, E. I. (July 2002). "Hidden Mass in the Asteroid Belt". Icarus. 158 (1): 98–105.
 */
void auto_generate_kuiper_belt(struct simulation_configuration_struct *simulation_configuration, int index) {
    double min_distance_to_sun =
            444445e7 + 24341000; // the distance between the inner edge of the Kuiper belt to the sun
    double max_distance_to_sun = 7.1e12; // the distance between the outer edge of the Kuiper belt to the sun
    double max_radius = 750e3; // smaller that Pallas
    double min_radius = 100e3;

    double start_x = -max_distance_to_sun;
    double increment = max_distance_to_sun * 2 / simulation_configuration->kuiper_belt;

    for (int i = 0; i < simulation_configuration->kuiper_belt; i++, index++) {
        char buffer[10];
        sprintf(buffer, "%d", index);
        strcpy(simulation_configuration->body_configurations[index].name, "KBO");
        strcat(simulation_configuration->body_configurations[index].name, buffer);
        simulation_configuration->body_configurations[index].radius = drandom(min_radius, max_radius);
        simulation_configuration->body_configurations[index].mass =
                drandom(10e3, 10e4) * pow(simulation_configuration->body_configurations[index].radius, 3);
        auto_generate_position(&simulation_configuration->body_configurations[index], start_x, min_distance_to_sun,
                               max_distance_to_sun);
        start_x += increment;
        simulation_configuration->body_configurations[index].velocity_x = 0;
        simulation_configuration->body_configurations[index].velocity_y = 45000;
        simulation_configuration->body_configurations[index].velocity_z = 0;
        simulation_configuration->body_configurations[index].active = true;
        simulation_configuration->body_configurations[index].type = ASTEROID;
    }
}


/*
* A simple configuration file reader, I don't think you will need to change this (but feel free if you want to!)
* It will parse the configuration file and set the appropriate configuration points that will then feed into the simulation
* setup. It is somewhat limited in its flexibility and you need to be somewhat careful about the configuration file format, 
* but is fine for our purposes
*/
void parseConfiguration(char *filename, struct simulation_configuration_struct *simulation_configuration) {
    initialiseSimulationConfiguration(simulation_configuration);
    FILE *f = fopen(filename, "r");
    int index;

    if (f == NULL) {
        printf("Error, can not open file %s for reading\n", filename);
        exit(-1);
    }
    char buffer[MAX_LINE_LENGTH];
    while ((fgets(buffer, MAX_LINE_LENGTH, f)) != NULL) {
        // If the string ends with a newline then remove this to make parsing simpler
        if (isspace(buffer[strlen(buffer) - 1])) buffer[strlen(buffer) - 1] = '\0';
        if (strlen(buffer) > 0) {
            if (buffer[0] == '#') continue; // This line is a comment so ignore
            if (hasValue(buffer)) {
                if (strstr(buffer, "NUM_ASTEROIDS_IN_BELT") != NULL)
                    simulation_configuration->asteroid_belt = getIntValue(buffer);
                if (strstr(buffer, "NUM_ASTEROIDS_IN_KUIPER") != NULL)
                    simulation_configuration->kuiper_belt = getIntValue(buffer);
                if (strstr(buffer, "NUM_TIMESTEPS") != NULL)
                    simulation_configuration->num_timesteps = getIntValue(buffer);
                if (strstr(buffer, "OUTPUT_FREQUENCY") != NULL)
                    simulation_configuration->output_frequency = getIntValue(buffer);
                if (strstr(buffer, "DISPLAY_PROGRESS_FREQUENCY") != NULL)
                    simulation_configuration->display_progess_frequency = getIntValue(buffer);
                if (strstr(buffer, "DT") != NULL) simulation_configuration->dt = getDoubleValue(buffer);
                if (strstr(buffer, "BODY_") != NULL) {
                    int bodyNumber = getEntityNumber(buffer);
                    if (bodyNumber >= 0) {
                        index = bodyNumber + 1;
                        // Exit if the exceed the maximum size of the bodies array
                        if (simulation_configuration->body_size < index) {
                            printf("You gave less body size than the number of boides in the configuration file!!!\n"
                                   "I can't believe you are so stingy!!!\n");
                            exit(-1);
                        }
                        simulation_configuration->body_configurations[bodyNumber].active = true;
                        if (strstr(buffer, "NAME") != NULL) {
                            char *equalsLocation = strchr(buffer, '=');
                            strcpy(simulation_configuration->body_configurations[bodyNumber].name, &equalsLocation[1]);
                        }
                        if (strstr(buffer, "_POSITION_X") != NULL)
                            simulation_configuration->body_configurations[bodyNumber].x = getDoubleValue(buffer);
                        if (strstr(buffer, "_POSITION_Y") != NULL)
                            simulation_configuration->body_configurations[bodyNumber].y = getDoubleValue(buffer);
                        if (strstr(buffer, "_POSITION_Z") != NULL)
                            simulation_configuration->body_configurations[bodyNumber].z = getDoubleValue(buffer);
                        if (strstr(buffer, "_MASS") != NULL)
                            simulation_configuration->body_configurations[bodyNumber].mass = getDoubleValue(buffer);
                        if (strstr(buffer, "_RADIUS") != NULL)
                            simulation_configuration->body_configurations[bodyNumber].radius = getDoubleValue(buffer);
                        if (strstr(buffer, "_VELOCITY_X") != NULL)
                            simulation_configuration->body_configurations[bodyNumber].velocity_x = getDoubleValue(
                                    buffer);
                        if (strstr(buffer, "_VELOCITY_Y") != NULL)
                            simulation_configuration->body_configurations[bodyNumber].velocity_y = getDoubleValue(
                                    buffer);
                        if (strstr(buffer, "_VELOCITY_Z") != NULL)
                            simulation_configuration->body_configurations[bodyNumber].velocity_z = getDoubleValue(
                                    buffer);
                        if (strstr(buffer, "_TYPE") != NULL) {
                            char *equalsLocation = strchr(buffer, '=');
                            simulation_configuration->body_configurations[bodyNumber].type = getBodyType(
                                    &equalsLocation[1]);
                        }
                    } else {
                        fprintf(stderr,
                                "Ignoring body configuration line '%s' as this is malformed and can not extract body number\n",
                                buffer);
                    }
                }
            } else {
                fprintf(stderr, "Ignoring configuration line '%s' as this is malformed\n", buffer);
            }
        }
    }
    fclose(f);
    /*
     * If the allocated size of the array is not enough, then exit the program
     * Even if the size is enough to store bodies for now, problems may occur due to generation of new bodies.
     * Hence, there will be a thought ful tip if there are not enough space for extra bodies
     */
    int temp = index + simulation_configuration->asteroid_belt + simulation_configuration->kuiper_belt;
    if (simulation_configuration->body_size < temp) {
        printf("What a stingy guy you are :(\nHow dare you initialise %d bodies with only %d　maximum body size?\n"
               "Try to define a larger body size and comeback\n", temp, simulation_configuration->body_size);
        exit(-1);
    } else if (simulation_configuration->body_size < temp + 50) {
        printf("Ok the body size is enough for now\n"
               "But problems may occur if too many new bodies are added\n"
               "And this program would fail because you are too stingy!!!\n");
    }
    /*
    * Generate the asteroids belt and the Kuiper Belt
    */
    if (simulation_configuration->kuiper_belt > 0)
        auto_generate_kuiper_belt(simulation_configuration, index);
    if (simulation_configuration->asteroid_belt >= 4)
        auto_generate_asteroid_belt(simulation_configuration, index + simulation_configuration->kuiper_belt);
}

/*
* Initialises the configuration data structure for this simulation, adds in some defaults if they are not specified and marks
* each body as inactive
*/
static void initialiseSimulationConfiguration(struct simulation_configuration_struct *simulation_configuration) {
    // Default values
    simulation_configuration->dt = 1.0;
    simulation_configuration->num_timesteps = 1000;
    simulation_configuration->output_frequency = 10;
    simulation_configuration->display_progess_frequency = 10000;
    simulation_configuration->asteroid_belt = ASTEROID_BELT; // Initialise the number of asteroids in the asteroid belt
    simulation_configuration->kuiper_belt = KUIPER_BELT; // Initialise the number of asteroids in the Kuiper Belt

    // Allocate for bodies according to the input or default configuration
    simulation_configuration->body_configurations = (struct body_config_struct *) malloc(
            sizeof(struct body_config_struct) * simulation_configuration->body_size);

    // Now set each body as inactive
    for (int i = 0; i < simulation_configuration->body_size; i++) {
        simulation_configuration->body_configurations[i].name[0] = '\0';
        simulation_configuration->body_configurations[i].active = false;
    }
}

static void auto_generate_position(struct body_config_struct *body, double x, double low, double high) {
    body->x = x;
    if (x > low || x < -low)
        body->y = drandom(0, sqrt(pow(high, 2) - pow(x, 2)));
    else
        body->y = drandom(sqrt(pow(low, 2) - pow(x, 2)), sqrt(pow(high, 2) - pow(x, 2)));
    if (rand() % 2 == 0)
        body->y = -body->y;
    body->z = 0;
}

/* 
* A helper function to parse a string with an underscore in it, this will extract the number after the underscore
* as we use this in the configuration file for setting numbers of bodies in the configuration
*/
static int getEntityNumber(char *sourceString) {
    char *underScoreLocation = strchr(sourceString, '_');
    if (underScoreLocation != NULL) {
        char *secondUnderScoreLocation = strchr(underScoreLocation + 1, '_');
        if (secondUnderScoreLocation != NULL) {
            int size_diff = secondUnderScoreLocation - underScoreLocation;
            char int_key[size_diff];
            strncpy(int_key, &underScoreLocation[1], size_diff - 1);
            return atoi(int_key);
        }
    }
    return -1;
}

/*
* From a string will extract the integer value and return this, or -1 if none is found
*/
static int getIntValue(char *sourceString) {
    char *equalsLocation = strchr(sourceString, '=');
    if (equalsLocation != NULL) {
        return atoi(&equalsLocation[1]);
    }
    return -1;
}

/*
* From a string will extract the double value and return this, of -1 if none is found
*/
static double getDoubleValue(char *sourceString) {
    char *equalsLocation = strchr(sourceString, '=');
    if (equalsLocation != NULL) {
        return atof(&equalsLocation[1]);
    }
    return -1;
}

/*
* Determines if a string has an equals in it or not (e.g. is there a value specified at this line?)
*/
static bool hasValue(char *sourceString) {
    return strchr(sourceString, '=') != NULL;
}

/*
 * Generate a random double number that sits in [low, high]
 */
static double drandom(double low, double high) {
    return ((double) rand() / (double) (RAND_MAX)) * (high - low) + low;
}

/*
* Maps from the string to the body type
*/
static enum body_type_enum getBodyType(char *sourceString) {
    if (strcmp(sourceString, "SUN") == 0) return SUN;
    if (strcmp(sourceString, "PLANET") == 0) return PLANET;
    if (strcmp(sourceString, "MOON") == 0) return MOON;
    if (strcmp(sourceString, "ASTEROID") == 0) return ASTEROID;
    if (strcmp(sourceString, "COMET") == 0) return COMET;
    return UNKNOWN;
}
