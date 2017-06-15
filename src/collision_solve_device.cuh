#ifndef COLLISION_SOLVE_DEVICE
#define COLLISION_SOLVE_DEVICE

#include "project_typedefs.hpp"


void detect_collisions_GPU_naive(Particle * particles, 
                                bool * output_collisions, 
                                unsigned int num_particles, 
                                float * comp_time, 
                                Particle * dev_particles,
                                bool * dev_output_collisions );

void detect_collisions_GPU_optimized1(Particle * particles, 
									bool * output_collisions, 
									unsigned int num_particles, 
									float * comp_time,
									Particle * dev_particles,
                                	bool * dev_output_collisions,
                                	ParticleBound * bounds_x,
                                	ParticleBound * bounds_y,
                                	int * dev_active_particles,
                                	int * dev_active_particles_len );

// void detect_collisions_GPU_optimized2(Particle * particles, 
// 									bool * output_collisions, 
// 									unsigned int num_particles, 
// 									float * comp_time,
// 									Particle * dev_particles,
//                                 	bool * dev_output_collisions );



#endif
