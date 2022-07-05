#ifndef CONFIGURATION_INCLUDE
#define CONFIGURATION_INCLUDE

#include <stdbool.h>
#include "simulation_support.h"

// Maximum number of history entries allowed before writing to file
#define MAX_HISTORY_SIZE 10000

// Maximum number of bodies that can be configured
#define MAX_BODY_CONFIGS 100

// Default number of asteroids in asteroids belt between Mars and Jupiter
#define ASTEROID_BELT 0

// Default number of asteroids in the kuiper belt between Mars and Jupiter
#define KUIPER_BELT 0

// Configuration of each body as read from the configuration file
// this is separate from the structure used when actually running the code
struct body_config_struct {
  char name[40];
  double x, y, z;
  double mass, radius;
  double velocity_x, velocity_y, velocity_z;
  bool active;
  enum body_type_enum type;
};

// Overall configuration of the simulation
struct simulation_configuration_struct {
  double dt;
  int body_size, asteroid_belt, kuiper_belt;
  int num_timesteps, output_frequency, display_progess_frequency;
  struct body_config_struct *body_configurations;
};

void parseConfiguration(char*, struct simulation_configuration_struct*);

#endif