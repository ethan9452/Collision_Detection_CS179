
#include "collision_solve_device.cuh"
// #include "project_typedefs.hpp"

#include <cstdio>
#include <cuda_runtime.h>
#include <time.h> 
#include <iostream>
#include <climits>
#include <algorithm>    // std::sort
#include <math.h>
#include <vector>

#include <thrust/sort.h>
#include <thrust/execution_policy.h>

#define BLOCKS              512
#define THREADS_PER_BLOCK   512

#define PRINT_TIMES         0

#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
    if (code != cudaSuccess) 
    {
        fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
        exit(code);
    }
}

bool particles_colliding_g(Particle p1, Particle p2) {
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




__global__
void detect_collisions_GPU_naive_kernel (   Particle * dev_particles,
                                            bool * dev_output_collisions,
                                            unsigned int num_particles ) {

    unsigned pidx1 = blockIdx.x * blockDim.x + threadIdx.x;

    Particle * p1;
    Particle * p2;
    bool col;

    

    while(pidx1 < num_particles) {

        for(unsigned int pidx2 = pidx1 + 1; pidx2 < num_particles; pidx2++) {

            p1 = dev_particles + pidx1;
            p2 = dev_particles + pidx2; 

            col = ((p1->x - p2->x) * (p1->x - p2->x)) + ((p1->y - p2->y) * (p1->y - p2->y)) <
                        (p1->radius + p2->radius) * (p1->radius + p2->radius);      

            if(col == true) {
                dev_output_collisions[(pidx1 * num_particles) + pidx2] = true;
                dev_output_collisions[(pidx2 * num_particles) + pidx1] = true;
            }
        }

        pidx1 += blockDim.x * gridDim.x;
    }

}


/*
Implentation of naive algorithm on GPU.


Index (x, y) is indexed by arr[(x * num_particles) + y]

*/
void detect_collisions_GPU_naive(Particle * particles, 
                                bool * output_collisions, 
                                unsigned int num_particles, 
                                float * comp_time, 
                                Particle * dev_particles,
                                bool * dev_output_collisions ) {


    const clock_t begin_time = clock();


#if PRINT_TIMES == 1
    const clock_t copy_start_time = clock(); 
#endif

    // Set values for data on GPU
    gpuErrchk(cudaMemset(dev_output_collisions, (int)false, sizeof(bool) * num_particles * num_particles));
    

#if PRINT_TIMES == 1
    cudaDeviceSynchronize();
    float cpoy_runtime = float( clock () - copy_start_time ) /  CLOCKS_PER_SEC;  
    printf("First copy takes %fs\n", cpoy_runtime);
#endif



#if PRINT_TIMES == 1
    const clock_t kernel_start_time = clock();
#endif
    
    // Call kernel
    detect_collisions_GPU_naive_kernel<<<BLOCKS, THREADS_PER_BLOCK>>>(dev_particles, dev_output_collisions, num_particles);
    
#if PRINT_TIMES == 1
    cudaDeviceSynchronize();
    float kernel_runtime = float( clock () - kernel_start_time ) /  CLOCKS_PER_SEC;  
    printf("Kernel takes %fs\n", kernel_runtime);
#endif

    // check for an error in kernal call
    cudaError err2 = cudaGetLastError();
    if(err2 != cudaSuccess) {
        printf("%s\n", cudaGetErrorString(err2));
    }

#if PRINT_TIMES == 1
    const clock_t cpoy_back_start_time = clock(); 
#endif    

    // Copy data back
    gpuErrchk(cudaMemcpy(output_collisions, dev_output_collisions, sizeof(bool) * num_particles * num_particles, cudaMemcpyDeviceToHost));
    
#if PRINT_TIMES == 1
    cudaDeviceSynchronize(); 
    float copy_back_runtime = float( clock () - cpoy_back_start_time ) /  CLOCKS_PER_SEC;  
    printf("Copy back takes %fs\n", copy_back_runtime);
#endif

    *comp_time = float( clock () - begin_time ) /  CLOCKS_PER_SEC;
}


__global__
void detect_collisions_GPU_optimized1_kernal_s1(  unsigned int num_particles, 
                                                    Particle * dev_particles,       // len: num_particles
                                                    bool * dev_output_collisions,   // len: num_particles^2. Index (x, y) is indexed by arr[(x * num_particles) + y]
                                                    ParticleBound * bounds_x,       // len: num_particles * 2
                                                    ParticleBound * bounds_y,       // len: num_particles * 2
                                                    int * dev_active_particles,     // len: num_particles
                                                    int * dev_active_particles_len, // a single int
                                                    float * x_sum,
                                                    float * y_sum)
{

    extern __shared__ float locations_for_sum[]; // first 'THREADS_PER_BLOCK' are x, second 'THREADS_PER_BLOCK' are y
                                                 // we do redection to find sum (and therfore mean) of x and y positions
    locations_for_sum[threadIdx.x] = 0.; // set x to 0
    locations_for_sum[threadIdx.x + THREADS_PER_BLOCK] = 0.; // set y to 0

    unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;

    // Fill bounds_x and bound_y
    while(i < num_particles) {

        bounds_x[2 * i].position = dev_particles[i].x - dev_particles[i].radius;
        bounds_x[2 * i].is_begin = true;
        bounds_x[2 * i].particle_idx = i;

        bounds_x[2 * i + 1].position = dev_particles[i].x + dev_particles[i].radius;
        bounds_x[2 * i + 1].is_begin = false;
        bounds_x[2 * i + 1].particle_idx = i;

        locations_for_sum[threadIdx.x] = dev_particles[i].x;

        bounds_y[2 * i].position = dev_particles[i].y - dev_particles[i].radius;
        bounds_y[2 * i].is_begin = true;
        bounds_y[2 * i].particle_idx = i;

        bounds_y[2 * i + 1].position = dev_particles[i].y + dev_particles[i].radius;
        bounds_y[2 * i + 1].is_begin = false;
        bounds_y[2 * i + 1].particle_idx = i;

        locations_for_sum[threadIdx.x + THREADS_PER_BLOCK] = dev_particles[i].y;


        __syncthreads();
        // Implement reduction to get sums
        for(uint s = blockDim.x / 2; s > 0; s>>=1) {
            if(threadIdx.x < s) {
                locations_for_sum[threadIdx.x] += locations_for_sum[threadIdx.x + s];
                locations_for_sum[threadIdx.x + THREADS_PER_BLOCK] += locations_for_sum[threadIdx.x + THREADS_PER_BLOCK + s];
            }
            __syncthreads();
        }

        if(threadIdx.x == 0) {
            atomicAdd(x_sum, locations_for_sum[0]);
            atomicAdd(y_sum, locations_for_sum[THREADS_PER_BLOCK]);
        }
        __syncthreads();

        i += blockDim.x * gridDim.x;
    }

}

__global__
void detect_collisions_GPU_optimized1_kernal_s2(  unsigned int num_particles, 
                                                    Particle * dev_particles,       // len: num_particles
                                                    bool * dev_output_collisions,   // len: num_particles^2. Index (x, y) is indexed by arr[(x * num_particles) + y]
                                                    ParticleBound * bounds_x,       // len: num_particles * 2
                                                    ParticleBound * bounds_y,       // len: num_particles * 2
                                                    int * dev_active_particles,     // len: num_particles
                                                    int * dev_active_particles_len, // a single int
                                                    float * x_mean,
                                                    float * y_mean,
                                                    float * x_var,
                                                    float * y_var )
{

    extern __shared__ float vars_shared[]; // The first 'THREADS_PER_BLOCK' elements are for x, second are for y

    unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;

    // Turn sum into mean
    if(i == 0) {
        *x_mean = *x_mean / (float) num_particles;
        *y_mean = *y_mean / (float) num_particles;
    }
    __syncthreads();


    // Find variance
    while(i < num_particles * 2) {

        vars_shared[threadIdx.x] = (bounds_x[i].position - *x_mean) * (bounds_x[i].position - *x_mean); // x axis
        vars_shared[threadIdx.x + THREADS_PER_BLOCK] = (bounds_y[i].position - *y_mean) * (bounds_y[i].position - *y_mean); // y axis

        __syncthreads();

        // Reduction
        for(uint s = blockDim.x / 2; s > 0; s>>=1) {
            if(threadIdx.x < s) {
                vars_shared[threadIdx.x] += vars_shared[threadIdx.x + s];
                vars_shared[threadIdx.x + THREADS_PER_BLOCK] += vars_shared[threadIdx.x + THREADS_PER_BLOCK + s];
            }
            __syncthreads();
        }

        if(threadIdx.x == 0) {
            atomicAdd(x_var, vars_shared[0]);
            atomicAdd(y_var, vars_shared[THREADS_PER_BLOCK]);
        }
        __syncthreads();


        i += blockDim.x * gridDim.x;
    }

}

void detect_collisions_GPU_optimized1(Particle * particles, 
                                    bool * output_collisions, 
                                    unsigned int num_particles, 
                                    float * comp_time,
                                    Particle * dev_particles,
                                    bool * dev_output_collisions,
                                    ParticleBound * bounds_x,
                                    ParticleBound * bounds_y,
                                    int * dev_active_particles, // indexes (wrt 'particles') of the active particles
                                    int * dev_active_particles_len ) {

    // Not including malloc in the time measurement bc in practice, we would only malloc once. Everything we are 
    // tracking time for is something that would be executed in a loop
    float * dev_x_mean; 
    float * dev_y_mean;
    gpuErrchk(cudaMalloc((void**)&dev_x_mean, sizeof(float)));
    gpuErrchk(cudaMalloc((void**)&dev_y_mean, sizeof(float)));

    gpuErrchk(cudaMemset(dev_x_mean, 0, sizeof(float)));
    gpuErrchk(cudaMemset(dev_y_mean, 0, sizeof(float)));

    float * dev_x_var; 
    float * dev_y_var;
    gpuErrchk(cudaMalloc((void**)&dev_x_var, sizeof(float)));
    gpuErrchk(cudaMalloc((void**)&dev_y_var, sizeof(float)));

    gpuErrchk(cudaMemset(dev_x_var, 0, sizeof(float)));
    gpuErrchk(cudaMemset(dev_y_var, 0, sizeof(float)));

    const clock_t begin_time = clock();


    // Set initial values for data on GPU
    gpuErrchk(cudaMemset(dev_output_collisions, (int)false, sizeof(bool) * num_particles * num_particles));


    // Call kernel to set bounds arrays and calculate sums (of x and y positions) for means.
    detect_collisions_GPU_optimized1_kernal_s1<<<BLOCKS, THREADS_PER_BLOCK, sizeof(float) * THREADS_PER_BLOCK * 2>>>(num_particles, 
        dev_particles, dev_output_collisions, bounds_x, bounds_y, dev_active_particles, dev_active_particles_len, dev_x_mean, dev_y_mean);

    // check for an error in kernal call
    cudaError err1 = cudaGetLastError();
    if(err1 != cudaSuccess) {
        printf("%s\n", cudaGetErrorString(err1));
    }


    /// Call kernel to find variance in x and y positions 
    detect_collisions_GPU_optimized1_kernal_s2<<<BLOCKS, THREADS_PER_BLOCK, sizeof(float) * THREADS_PER_BLOCK * 2>>>(num_particles, 
        dev_particles, dev_output_collisions, bounds_x, bounds_y, dev_active_particles, dev_active_particles_len, dev_x_mean, dev_y_mean, dev_x_var, dev_y_var);

    // check for an error in kernal call
    cudaError err2 = cudaGetLastError();
    if(err2 != cudaSuccess) {
        printf("%s\n", cudaGetErrorString(err2));
    }

    float x_var;
    float y_var;
    gpuErrchk(cudaMemcpy(&x_var, dev_x_var, sizeof(float), cudaMemcpyDeviceToHost));
    gpuErrchk(cudaMemcpy(&y_var, dev_y_var, sizeof(float), cudaMemcpyDeviceToHost));


    // Choose higher variance
    ParticleBound * chosen_bounds; // Device
    if(x_var > y_var) {
        chosen_bounds = bounds_x;
    }
    else {
        chosen_bounds = bounds_y;
    }


    // Sort the chosen_bounds array
    // For this, I used the Cuda Thrust library, which has a parallel sorting function.
    thrust::sort(thrust::device, chosen_bounds, chosen_bounds + num_particles * 2);

    // check for an error in kernal call
    cudaError err3 = cudaGetLastError();
    if(err3 != cudaSuccess) {
        printf("%s\n", cudaGetErrorString(err3));
    }

    ParticleBound * bounds_chosen = new ParticleBound[2*num_particles]; // Host
    cudaMemcpy(bounds_chosen, chosen_bounds, sizeof(ParticleBound) * num_particles * 2, cudaMemcpyDeviceToHost);

    // List of particle_idx's that are 'active', ie: we are still inside its bounds
    std::vector<unsigned int> active_particles; 

    // Iterate through the sorted list of bounds
    // Note: did not make this into a kernel for reasons discussed in the readme
    for(unsigned long int i = 0; i < 2*num_particles; i++) {
        if(bounds_chosen[i].is_begin == true) {
            // Elements of active_particles are all potential collisions.
            // Check with each element of active_particles
            for(unsigned int active_idx = 0; active_idx < active_particles.size(); active_idx++) {
                
                if(particles_colliding_g(particles[bounds_chosen[i].particle_idx], particles[active_particles[active_idx]])) {

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


    // Copy data back to host
    // gpuErrchk(cudaMemcpy(output_collisions, dev_output_collisions, sizeof(bool) * num_particles * num_particles, cudaMemcpyDeviceToHost));


    *comp_time = float( clock () - begin_time ) /  CLOCKS_PER_SEC;

    cudaFree(dev_x_mean);
    cudaFree(dev_y_mean);
    cudaFree(dev_x_var);
    cudaFree(dev_y_var);

}

// void detect_collisions_GPU_optimized2(Particle * particles, 
//                                     bool * output_collisions, 
//                                     unsigned int num_particles, 
//                                     float * comp_time) {

// }




