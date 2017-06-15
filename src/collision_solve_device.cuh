#ifndef COLLISION_SOLVE_DEVICE
#define COLLISION_SOLVE_DEVICE

#include "project_typedefs.hpp"



void detect_collisions_GPU_naive(Particle * particles, 
								bool * output_collisions, 
								unsigned int num_particles, 
								float * comp_time);

void detect_collisions_GPU_optimized1(Particle * particles, 
									bool * output_collisions, 
									unsigned int num_particles, 
									float * comp_time);

void detect_collisions_GPU_optimized2(Particle * particles, 
									bool * output_collisions, 
									unsigned int num_particles, 
									float * comp_time);



#endif
