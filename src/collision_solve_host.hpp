
#ifndef COLLISION_SOLVE_HOST
#define COLLISION_SOLVE_HOST

#include "project_typedefs.hpp"
#include <time.h> 
#include <iostream>
#include <climits>
#include <algorithm>    // std::sort
#include <vector>
#include <math.h>
#include <cstring>



using namespace std;

bool particles_colliding(Particle p1, Particle p2);

void detect_collisions_CPU_naive(Particle * particles, 
								bool * output_collisions, 
								unsigned int num_particles, 
								float * comp_time);

void detect_collisions_CPU_optimized1(Particle * particles, 
									bool * output_collisions, 
									unsigned int num_particles, 
									float * comp_time);

void detect_collisions_CPU_optimized2(Particle * particles, 
									bool * output_collisions, 
									unsigned int num_particles, 
									float * comp_time);


#endif