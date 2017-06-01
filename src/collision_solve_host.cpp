
#include "collision_solve_host.hpp"



#define DEBUG 0 // 1 for debug, 0 for prod

/*
Code for the CPU solution of collision detection

*/

/*
Returns true of p1 and p2 are colliding. (Strictly, if they are intersecting or
tangent to each other)

*/
bool particles_colliding(Particle p1, Particle p2) {
	// 
	return ((p1.x - p2.x) * (p1.x - p2.x)) + ((p1.y - p2.y) * (p1.y - p2.y)) <= 
			(p1.radius + p2.radius) * (p1.radius + p2.radius);
}


/*
** NOTE: max accepted number of particles is around 10000
	after that I get seg faults


On the CPU, runs a naive algorithm to find collisions: does a double
for loop through the particle list.

Index (x, y) is indexed by arr[(x * num_particles) + y]
*/
void detect_collisions_CPU_naive(Particle * particles, bool * output_collisions, unsigned int num_particles, float * comp_time) {
	const clock_t begin_time = clock();

	for(unsigned int pidx1 = 0; pidx1 < num_particles; pidx1++) {
		#if DEBUG == 1
		cout << "Num particles" << num_particles;
		cout << "\nChecking pidx1: " << pidx1 << "\n";
		#endif
		for(unsigned int pidx2 = pidx1 + 1; pidx2 < num_particles; pidx2++) {
			if(particles_colliding(particles[pidx1], particles[pidx2])) {
				output_collisions[(pidx1 * num_particles) + pidx2] = true;
				output_collisions[(pidx2 * num_particles) + pidx1] = true;
			}
		}
	}

	*comp_time = float( clock () - begin_time ) /  CLOCKS_PER_SEC;
}


/*
Sort and Sweep Alg based off:
	https://www.toptal.com/game/video-game-physics-part-ii-collision-detection-for-solid-objects

Sort and Sweep one only one axis	

Pseudocode:
	- bounds_x: [(x position, begin or end, particle idx), (...), ...]
	- bounds_y: [(y position, begin or end, particle idx), (...), ...]
	- iterate through particles to fill the arrays bounds_x and bounds_y
	- find variance of position in bounds_x or bounds_y
	- iterate through the bounds array with higher variance:
		- 
		

Index (x, y) is indexed by arr[(x * num_particles) + y]
*/
void detect_collisions_CPU_optimized1(Particle * particles, bool * output_collisions, unsigned int num_particles, float * comp_time) {
	const clock_t begin_time = clock();





	*comp_time = float( clock () - begin_time ) /  CLOCKS_PER_SEC;
}






