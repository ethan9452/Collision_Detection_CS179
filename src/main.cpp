// Ethan Lo
// CS179 Project
// Collision Detection

/*
    This code generates an array of partices and outputs which of them are intersecting.
    The parameters of this simulation (specified by user) are:
        - number of particles, num_particles
        - circular particles with radii in the range [min_rad, max_rad], inclusive
        - box_x by box_y box that the particles exist in

*/


// #include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdlib.h> // srand, rand
#include <time.h> 
#include <climits>
#include <math.h>

#include "project_typedefs.hpp"
#include "collision_solve_host.hpp"
#include "collision_solve_device.cuh"

using namespace std;

/*
Prints the contents of 'particles'
*/
void print_particle_array(Particle * particles, unsigned int len) {
    cout << "\n";
    for(unsigned int i = 0; i < len; i++) {
        cout << "{x : " << particles[i].x << ", y : " << particles[i].y 
             << ", rad : " << particles[i].radius << "}\n";
    }
}

/*
Prints the pairs of particles that are colliding. 

*** Assumes the matrix is symetrical along diagonal axis. This should be true 
since the pair (p1,p2) is the same pair as (p2,p1)
*/
void print_colliding_pairs(bool * output_collisions, unsigned int num_particles) {
    cout << "\n";
    for(unsigned int pidx1 = 0; pidx1 < num_particles; pidx1++) {
        for(unsigned int pidx2 = pidx1 + 1; pidx2 < num_particles; pidx2++) {
            if(output_collisions[(pidx1 * num_particles) + pidx2] == true) {
                cout << "(" << pidx1 << ", " << pidx2 << ")";
            }
        }
    }
    cout << "\n";
}

unsigned long int count_colliding_pairs(bool * output_collisions, unsigned int num_particles) {
    unsigned long int ret = 0;
    for(unsigned int pidx1 = 0; pidx1 < num_particles; pidx1++) {
        for(unsigned int pidx2 = pidx1 + 1; pidx2 < num_particles; pidx2++) {
            if(output_collisions[(pidx1 * num_particles) + pidx2] == true) {
                ret++;
            }
        }
    }
    return ret;
}

/*
Prints visualization of collision output array

Index (x, y) is indexed by arr[(x * num_particles) + y]
*/
void print_collision_array(bool * output_collisions, unsigned int num_particles) {
    for(unsigned int y = 0; y < num_particles; y++) {
        for(unsigned int x = 0; x < num_particles; x++) {
            cout << output_collisions[(x * num_particles) + y];
        }
        cout << "\n";
    }
}


int main(int argc, char **argv) {


    /* initialize random seed: */
    srand (time(NULL));

    bool DEBUG_MODE;
    bool GRAPH_MODE;

    // Parse options
    if(argc == 2) {
        if(strcmp (argv[1], "-d") == 0) {
            DEBUG_MODE = true;
            GRAPH_MODE = false;
        }
        else if(strcmp (argv[1], "-g") == 0) {
            DEBUG_MODE = false;
            GRAPH_MODE = true;
        }
        else if(strcmp (argv[1], "-dg") == 0 || strcmp (argv[1], "-gd") == 0) {
            DEBUG_MODE = true;
            GRAPH_MODE = true;
        }
        else {
            printf("Usage: ./collision [-dg]\n    -d specifies debug mode\n");
            return 1;
        }
    }
    else if(argc == 1) {
        DEBUG_MODE = false;
        GRAPH_MODE = false;
    }
    else {
        // printf("argc: %i\n", argc);
        printf("Usage: ./collision [-dg]\n    -d specifies debug mode\n");
        return 1;
    }


    unsigned int num_particles;
    float min_rad, max_rad, box_x, box_y;
    int cluster; 



    // Get user input
    cout << "Enter number of particles: ";
    cin >> num_particles;
    cout << "Enter minimum particle radius: ";
    cin >> min_rad;
    cout << "Enter maximum particle radius: ";
    cin >> max_rad;
    cout << "Enter particle holding box's x length: ";
    cin >> box_x;
    cout << "Enter particle holding box's y length: ";
    cin >> box_y;
    cout << "Cluster points around center, or uniformly distributed? 1 for cluster, 0 for uniform:"; // TODO REMOVE cluster if i can't find the bug
    cin >> cluster;

    // Cap number of particles at 60000 to prevent overflow error
    // 60000^2 is close to the ULONG_MAX
    if(num_particles > 60000) {
        cout << "Max particle count is 60000, to prevent unsigned long overflow.\n";
        return -1;
    }

    // Check for illogical user input
    if(min_rad < 0 || max_rad < 0 || box_x < 0 || box_y < 0) {
        cout << "Bad input: no NEGATIVE lengths\n";
        return -1;
    }
    if(max_rad*2 > box_x || max_rad*2 > box_y) {
        cout << "Bad input: max particle diameter larger than box dimensions\n";
        return -1;
    }

    cout << "\nGenerating " << num_particles << " particles with radii in the range " 
        << min_rad << " to " << max_rad << " (inclusive), inside a bounding box with dimemsions " 
        << box_x << " by " << box_y << "\n\n";


    // Generate array of circular particles
    Particle * particles = (Particle *) malloc(sizeof(Particle) * num_particles);
    if(particles == NULL) {
        cout << "malloc for particles failed. failed to allocate " << (sizeof(Particle) * num_particles)
                << "bytes.\n";
        return -1;
    }


    for(unsigned int i = 0; i < num_particles; i++) {
        // Random float from 0.0 to 1.0
        float r1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float r2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float r3 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        particles[i].radius = (r1 * (max_rad - min_rad)) + min_rad;

        // possible range for center: [radius, box_len - radius]
        if(cluster == 0) {
            float x_range = box_x - (2 * particles[i].radius); 
            particles[i].x = (r2 * x_range) + particles[i].radius;

            float y_range = box_y - (2 * particles[i].radius);
            particles[i].y = (r3 * y_range) + particles[i].radius;
        }
        else {
            // Used for choosing positive or negative
            // float r4 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            // float r5 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

            // Center (x,y) = (box_x / 2., box_y / 2.)
            // Can use a uniform^2 or uniform^3, 4, ... distribution for heavier density in middle

            float x_range = box_x - (2 * particles[i].radius);
            float y_range = box_y - (2 * particles[i].radius);


            // cout << "x_range: " << x_range << "\n";
            // cout << "y_range: " << y_range << "\n"; // TODO DELETE

            float radius; // max radius is min(x_range, y_range)
            float max_rad;
            if(x_range < y_range) {
                max_rad = x_range / 2.;
            }
            else {
                max_rad = y_range / 2.;
            }

            // cout << "max_rad: " << max_rad << "\n"; // TODO DELETE



            // radius = (r2 * max_rad) * (r2 * max_rad) / max_rad; // Uniform squared
            if(max_rad == 0.) {
                radius = 0.;
            }
            else {
                radius = (r2 * max_rad) * (r2 * max_rad) * (r2 * max_rad) / (max_rad * max_rad); // Uniform cubed
            }
            
            

            // cout << "radius: " << radius << "\n"; // TODO DELETE

            float radians = r3 * 6.28318530718; // Between 0 and 2pi

            // cout << "radians: " << radians << "\n"; // TODO DELETE

            // x = r*cos(t)
            // y = r*sin(t)
            particles[i].x = (radius * cos(radians)) + (x_range / 2.) + particles[i].radius;

            // cout << "particles[i].x: " << particles[i].x << "\n"; // TODO DELETE



            particles[i].y = (radius * sin(radians)) + (y_range / 2.) + particles[i].radius;

            // cout << "particles[i].y: " << particles[i].y << "\n"; // TODO DELETE

            // float half_x_range = x_range / 2.;
            // float uniform_squared = (r2 * half_x_range) * (r2 * half_x_range) / (half_x_range);
            // // Choose negative or positive
            // if(r4 > .5) {
            //     particles[i].x = (box_x / 2.) + uniform_squared;
            // }
            // else {
            //     particles[i].x = (box_x / 2.) - uniform_squared;
            // }

            // float y_range = box_y - (2 * particles[i].radius);
            // uniform_squared = (r3 * y_range / 2.) * (r3 * y_range / 2.) / (y_range / 2.);
            // // Choose negative or positive
            // if(r5 > .5) {
            //     particles[i].y = (box_y / 2.) + uniform_squared;
            // }
            // else {
            //     particles[i].y = (box_y / 2.) - uniform_squared;
            // }


        }

        
    }


    // Check if any particles created are outside the box (debugging)
    for(unsigned int i = 0; i < num_particles; i++) {
        if(particles[i].x - particles[i].radius < 0) {
            cout << "Particle " << i << " sticking of the left side.";
            return -1;
        }
        if(particles[i].x + particles[i].radius > box_x) {
            cout << "Particle " << i << " sticking of the right side.";
            return -1;
        }
        if(particles[i].y - particles[i].radius < 0) {
            cout << "Particle " << i << " sticking of the bottom side.";
            return -1;
        }
        if(particles[i].y + particles[i].radius > box_y) {
            cout << "Particle " << i << " sticking of the top side.";
            return -1;
        }
        if(isnan(particles[i].x) || isnan(particles[i].y)) {
            cout << particles[i].x << "," << particles[i].y << " something is NaN!!!!\n";
            return -1;
        }   
    }
    

    // Print particles for debugging
    if(DEBUG_MODE) { print_particle_array(particles, num_particles); cout << "\n\n"; }

    /*********** CPU IMPLEMENTATION ***********/
    cout << "----------------------------------------------\n";
    cout << "Running collision detection algs...\n";

    // Naive CPU Implementation
    /*
    2D array where arr[x,y] = 1 when the particles indexed by x and y collide. 0 when
    they do not collide. 

    Index (x, y) is indexed by arr[(x * num_particles) + y]
    */
    bool * collisions_CPU_naive = (bool *) malloc(sizeof(bool) * num_particles * num_particles);
    if(collisions_CPU_naive == NULL) {
        cout << "malloc for collisions_CPU_naive failed. failed to allocate " << (sizeof(bool) * num_particles * num_particles)
                << "bytes.\n";
        return -1;
    }
    memset(collisions_CPU_naive, 0, sizeof(bool) * num_particles * num_particles);
    float comp_time_CPU_naive = -1.;
    detect_collisions_CPU_naive(particles, collisions_CPU_naive, num_particles, &comp_time_CPU_naive);
    
    cout << "\nComputation time for Naive Alg, CPU: " << comp_time_CPU_naive << "s\n";
    cout << 1./comp_time_CPU_naive << " max frames per second\n";
    cout << "Number of colliding pairs: " << count_colliding_pairs(collisions_CPU_naive, num_particles) << "\n";

    if(DEBUG_MODE) {
        cout << "Colliding pairs:\n";
        print_colliding_pairs(collisions_CPU_naive, num_particles);
        print_collision_array(collisions_CPU_naive, num_particles);
        cout << "---------------\n";
    }



    // Optimized CPU Implementation 1
    bool * collisions_CPU_optimized = (bool *) malloc(sizeof(bool) * num_particles * num_particles);
    if(collisions_CPU_optimized == NULL) {
        cout << "malloc for collisions_CPU_optimized failed. failed to allocate " << (sizeof(bool) * num_particles * num_particles)
                << "bytes.\n";
        return -1;
    }    
    memset(collisions_CPU_optimized, 0, num_particles * num_particles);
    float comp_time_CPU_optimized = -1.;
    detect_collisions_CPU_optimized1(particles, collisions_CPU_optimized, num_particles, &comp_time_CPU_optimized);
    
    cout << "\n\nComputation time for Optimized Alg 1 (Sort and Sweep), CPU: " << comp_time_CPU_optimized << "s\n";
    cout << 1./comp_time_CPU_optimized << " max frames per second\n";
    cout << "Number of colliding pairs: " << count_colliding_pairs(collisions_CPU_optimized, num_particles) << "\n";

    if(DEBUG_MODE) {
        cout << "Colliding pairs:\n";
        print_colliding_pairs(collisions_CPU_optimized, num_particles);
        print_collision_array(collisions_CPU_optimized, num_particles);
        cout << "---------------\n";
    }


    // Optimized CPU implementation 2
    bool * collisions_CPU_optimized2 = (bool *) malloc(sizeof(bool) * num_particles * num_particles);
    if(collisions_CPU_optimized2 == NULL) {
        cout << "malloc for collisions_CPU_optimized2 failed. failed to allocate " << (sizeof(bool) * num_particles * num_particles)
                << "bytes.\n";
        return -1;
    }    
    memset(collisions_CPU_optimized2, 0, num_particles * num_particles);
    float comp_time_CPU_optimized2 = -1.;
    detect_collisions_CPU_optimized2(particles, collisions_CPU_optimized2, num_particles, &comp_time_CPU_optimized2);
    
    cout << "\n\nComputation time for Optimized Alg 2 (Quadtree), CPU: " << comp_time_CPU_optimized2 << "s\n";
    cout << 1./comp_time_CPU_optimized2 << " max frames per second\n";
    cout << "Number of colliding pairs: " << count_colliding_pairs(collisions_CPU_optimized2, num_particles) << "\n";

    if(DEBUG_MODE) {
        cout << "Colliding pairs:\n";
        print_colliding_pairs(collisions_CPU_optimized2, num_particles);
        print_collision_array(collisions_CPU_optimized2, num_particles);
        cout << "---------------\n";
    }


    /*********** GPU IMPLEMENTATION ***********/


    // ******** GPU input array has to be copied all at once (cannot be streamed)
    // since collision detection requires access to all particles - assuming the 
    // particle array is in no particular order.


    // Allocate space for GPU output




    /*********** CHECK RESULTS ***********/
    // Assume the CPU naive alg is correct, since it's hardest to mess up.

    cout << "\n----------------------------------------------\n";
    cout << "Checking results ...\n";

    // Check collisions_CPU_naive vs collisions_CPU_optimized
    bool cpu1_correct = true;
    for(unsigned long int i = 0; i < num_particles * num_particles; i++) {
        if(collisions_CPU_naive[i] != collisions_CPU_optimized[i]) {
            cout << "\n\nAlg Error: Optimized CPU alg 1 is wrong \n\n";
            cpu1_correct = false;
            break;
            // return -1;
        }
    }
    if(cpu1_correct) {
        cout << "\nOptimized CPU alg 1 matches naive implementation!\n";
    }
    

    // Check collisions_CPU_naive vs collisions_CPU_optimized2
    bool cpu2_correct = true;
    for(unsigned long int i = 0; i < num_particles * num_particles; i++) {
        if(collisions_CPU_naive[i] != collisions_CPU_optimized2[i]) {
            cout << "\n\nAlg Error: Optimized CPU alg 2 is wrong \n\n";
            cpu2_correct = false;
            break;
            // return -1;
        }
    }
    if(cpu2_correct) {
        cout << "\nOptimized CPU alg 2 matches naive implementation! \n\n";
    }


    cout << "\n\n";

    /*********** WRITE DATA TO FILE ***********/
    
    // Write data to ../output folder 
    if(GRAPH_MODE) {


        // Write particle data in the csv-form:
        // x_box,y_box
        // x, y, radius 
        // x, y, radius
        // ...
        remove( "../output/particle_data.csv" );
        ofstream particle_data_file;
        particle_data_file.open ("../output/particle_data.csv");
        particle_data_file << box_x << "," << box_y << "\n";
        for(int i = 0; i < num_particles; i++) {
            particle_data_file << particles[i].x << "," << particles[i].y << "," << particles[i].radius << "\n";
        }
        particle_data_file.close();

        if(num_particles > 1000) {
            cout << "You are about to plot over 1000 particles. Are you SURE you want to do that?\n";
            char yn;
            while(true) {
                cout << "y/n\n";
                cin >> yn;
                if(yn == 'y') {
                    break;
                }
                else if(yn == 'n') {
                    return 0;
                }
            }
            
        }

        system("python graph_it.py");

    }


    /*********** FREE STUFF ***********/
}









