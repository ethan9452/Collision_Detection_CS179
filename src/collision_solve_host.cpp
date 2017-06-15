
#include "collision_solve_host.hpp"



#define DEBUG 0 // Debug Mode
				// 0 prod
				// 1 - check constrains
				// 2 - print out state

/*
Code for the CPU solution of collision detection

*/

/*
Returns true of p1 and p2 are colliding. (Strictly, if they are intersecting or
tangent to each other)

*/
bool particles_colliding(Particle p1, Particle p2) {
	// 
	#if DEBUG == 1
	if(isnan(p1.x)) {
		cout << "Nan in particles_colliding()\n";
	}
	if(isnan(p2.x)) {
		cout << "Nan in particles_colliding()\n";
	}
	if(isnan(p1.y)) {
		cout << "Nan in particles_colliding()\n";
	}
	if(isnan(p2.y)) {
		cout << "Nan in particles_colliding()\n";
	}
	if(isnan(p1.radius)) {
		cout << "Nan in particles_colliding()\n";
	}
	if(isnan(p2.radius)) {
		cout << "Nan in particles_colliding()\n";
	}
	#endif


	return ((p1.x - p2.x) * (p1.x - p2.x)) + ((p1.y - p2.y) * (p1.y - p2.y)) <
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
#if DEBUG == 2
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
		- find all possible collisions (overlapping bounds):
			- when we find a boundary that starts a region, the reigon overlaps with 
				all the reigons in 'active reigons' list, so we check collisions for all those pairs.
			- then we we add it to the 'active reigons' list.
				- when we find the boundary that closes the above reigon, remove it from the 'active reigons' list.

* Compared to the naive algorithm, this alg performs best on a particle sparse reigon.


Index (x, y) is indexed by arr[(x * num_particles) + y]
*/
void detect_collisions_CPU_optimized1(Particle * particles, bool * output_collisions, 
									unsigned int num_particles, float * comp_time) {

	const clock_t begin_time = clock();
	
	// List of the particle boundaries
	ParticleBound * bounds_x = (ParticleBound *) malloc(sizeof(ParticleBound) * num_particles * 2);
	ParticleBound * bounds_y = (ParticleBound *) malloc(sizeof(ParticleBound) * num_particles * 2);
	memset(bounds_x, 0, sizeof(ParticleBound) * num_particles * 2);
	memset(bounds_y, 0, sizeof(ParticleBound) * num_particles * 2);

	// Iterate through particles to fill bounds
	// and calculate variance of positions. 
	float x_sum = 0.; 
	float y_sum = 0.;
	for(unsigned int i = 0; i < num_particles; i++) {
		bounds_x[2 * i].position = particles[i].x - particles[i].radius;
		bounds_x[2 * i].is_begin = true;
		bounds_x[2 * i].particle_idx = i;

		x_sum += (particles[i].x - particles[i].radius);

		bounds_x[2 * i + 1].position = particles[i].x + particles[i].radius;
		bounds_x[2 * i + 1].is_begin = false;
		bounds_x[2 * i + 1].particle_idx = i;

		x_sum += (particles[i].x + particles[i].radius);

		bounds_y[2 * i].position = particles[i].y - particles[i].radius;
		bounds_y[2 * i].is_begin = true;
		bounds_y[2 * i].particle_idx = i;

		y_sum += (particles[i].y - particles[i].radius);

		bounds_y[2 * i + 1].position = particles[i].y + particles[i].radius;
		bounds_y[2 * i + 1].is_begin = false;
		bounds_y[2 * i + 1].particle_idx = i;

		y_sum += (particles[i].y + particles[i].radius);
	}


	// Calculating Variance of x and y axis
	float x_mean = x_sum / (float) (num_particles * 2);
	float y_mean = y_sum / (float) (num_particles * 2);

	float x_var = 0.;
	float y_var = 0.;

	for(unsigned long int i = 0; i < 2*num_particles; i++) {
		x_var += (bounds_x[i].position - x_mean) * (bounds_x[i].position - x_mean);
		y_var += (bounds_y[i].position - y_mean) * (bounds_y[i].position - y_mean);
	}


	// Choose the axis with the larger variance to sweep/sort
	ParticleBound * bounds_chosen;
	if(x_var > y_var) {
		bounds_chosen = bounds_x;

#if DEBUG == 2
cout << "\nx-axis has more variance\n";
#endif
	}
	else {
		bounds_chosen = bounds_y;

#if DEBUG == 2
cout << "\ny-axis has more variance\n";
#endif
	}

#if DEBUG == 2
cout << "\nBEFORE SORT: position, is_begin, particle_idx \n";
for(unsigned long int i = 0; i < 2*num_particles; i++) {
	cout << bounds_chosen[i].position << ", " << bounds_chosen[i].is_begin << ", " << bounds_chosen[i].particle_idx << "\n";
}
cout << "\n";
#endif

	// Sort bounds_chosen based on bounds_chosen.position
	sort(bounds_chosen, bounds_chosen + num_particles*2);
	
#if DEBUG == 2
cout << "\nAFTER SORT: position, is_begin, particle_idx \n";
for(unsigned long int i = 0; i < 2*num_particles; i++) {
	cout << bounds_chosen[i].position << ", " << bounds_chosen[i].is_begin << ", " << bounds_chosen[i].particle_idx << "\n";
}
cout << "\n";
#endif


	// Now that bounds_chosen is sorted, iterate through and find the pairs of particles that 
	// are possible colliding. If they are possibly colliding, then check if they are colliding

	// List of particle_idx's that are 'active', ie: we are still inside its bounds
	vector<unsigned int> active_particles; 

	// Iterate through the sorted list of bounds
	for(unsigned long int i = 0; i < 2*num_particles; i++) {
		if(bounds_chosen[i].is_begin == true) {
			// Elements of active_particles are all potential collisions.
			// Check with each element of active_particles
			for(unsigned int active_idx = 0; active_idx < active_particles.size(); active_idx++) {

#if DEBUG == 2
cout << "possible collision between " << bounds_chosen[i].particle_idx << ", " << active_particles[active_idx] << "\n";
#endif
				if(particles_colliding(particles[bounds_chosen[i].particle_idx], particles[active_particles[active_idx]])) {
#if DEBUG == 2
cout << "YES! collision between " << bounds_chosen[i].particle_idx << ", " << active_idx << "\n";
#endif
					output_collisions[(bounds_chosen[i].particle_idx * num_particles) + active_particles[active_idx]] = true;
					output_collisions[(active_particles[active_idx] * num_particles) + bounds_chosen[i].particle_idx] = true;
				}
			}
			active_particles.push_back(bounds_chosen[i].particle_idx);

		}
		else {
			// Remove bounds_chosen[i].particle_idx from active_particles
			active_particles.erase(remove(active_particles.begin(), active_particles.end(), bounds_chosen[i].particle_idx), active_particles.end());
		}
		
	}



	free(bounds_x);
	free(bounds_y);

	*comp_time = float( clock () - begin_time ) /  CLOCKS_PER_SEC;
}

/*
Quadtree algorithm


*/
void detect_collisions_CPU_optimized2(Particle * particles, bool * output_collisions, 
									unsigned int num_particles, float * comp_time) {

	const clock_t begin_time = clock();


	// Init root QuadtreeNode
	// new ....

	




	*comp_time = float( clock () - begin_time ) /  CLOCKS_PER_SEC;
}





