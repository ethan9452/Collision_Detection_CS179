
#ifndef PROJECT_TYPEDEFS
#define PROJECT_TYPEDEFS

typedef struct {
    float radius;
    float x; // Coordinates of center
    float y;
} Particle;


// bounds_x/y: [(x/y position, begin or end, particle idx), (...), ...]
struct ParticleBoundForwardDecl;
typedef struct ParticleBoundForwardDecl {
	float position;
	bool is_begin; 
	unsigned int particle_idx;

	bool operator < (const ParticleBoundForwardDecl& other) const {
		return position < other.position;
	}

} ParticleBound;


// Holds the indexes of particles
struct QuadtreeNodeForwardDecl;
typedef struct QuadtreeNodeForwardDecl {
	unsigned int capacity; // max number of particles per node
	unsigned int num_particles; // current number of particles in node
	unsigned int * particle_idxs; // particles in node
	QuadtreeNodeForwardDecl * children; // Indexes of the childred correspond to the 4 cartesian quadrants
										// Always init to len 4
} QuadtreeNode;


// struct Particle {
//  float radius;
//  float x;
//  float y;
// };

#endif