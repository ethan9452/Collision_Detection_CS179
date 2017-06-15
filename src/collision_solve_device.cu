
#include "collision_solve_device.cuh"

#include <cstdio>
#include <cuda_runtime.h>
#include <time.h> 
#include <iostream>
#include <climits>
#include <algorithm>    // std::sort
#include <vector>
#include <math.h>








void detect_collisions_GPU_naive(Particle * particles, 
								bool * output_collisions, 
								unsigned int num_particles, 
								float * comp_time) {

}

void detect_collisions_GPU_optimized1(Particle * particles, 
									bool * output_collisions, 
									unsigned int num_particles, 
									float * comp_time) {

}

void detect_collisions_GPU_optimized2(Particle * particles, 
									bool * output_collisions, 
									unsigned int num_particles, 
									float * comp_time) {

}