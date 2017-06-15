https://github.com/ethan9452/Collision_Detection_CS179
CS179 Collision Detection Project

Installation/Usage
	Compile
		- cd into src folder
		- make
	Run
		- ./main [-dg]
		The d option prints debug output. It is especially helpful because for each algorithm, it displays a matrix of which particles are intersecting. This makes it easy to visually verify that the program is working as expected
		The g option will cause the program to output the particle data to a file, and then run a python script to display those particles using matplotlib. This another way one can verify the validity of the collision detection algorithms. 

	Dependencies
		- Cuda
		- python, matplotlib (if you want to display the particles visually)

	Specifying Dimensions
		Num-threads and num-blocks is hardcoded in collision_solve_device, so one can change it there.


Explanation of Program
	After the user begins the program, it is prompted to specify: number of particles, min/max particle radius, dimensions of the box the particles reside in, and whether to distribute the particles uniformly around the box, or have them clustered in the middle. 

	Based on the above parameters, an array of particles is randomly generated. Then multiple collision detection algorithms calculate which particles are colliding. After, the results are compared against each other to make sure they are all the same. Information about the times each algorithm takes is printed. 


Algorithms
	Naive Algorithm: checks every particle against every other particle to see if any collide. Implemented both CPU and GPU versions.

	Sort and Sweep Algorithm: this method attempts to reduce the number of comparisons by using spatial locality. For both the x and y axis, we add the boundaries of the shapes to an array (one array for x-axis, one for the y-axis). At the same time, we are summing the x and y positions so we can get the mean x position and mean y position. With the mean, we calculate the variance in position along each axis. Then, we examine the axis with higher variability and only check particles that intersect on the that axis. Since the axis has higher variability, there will be less comparisions. I learned about this algorithm from this website: https://www.toptal.com/game/video-game-physics-part-ii-collision-detection-for-solid-objects 

	Note that in the GPU version, we must traverse the axis in spatial order, so it does not make sense to parallelize this step. (I am referring to line 378 in collision_solve_device.cu: "for(unsigned long int i = 0; i < 2*num_particles; i++) "). 


Expected Results and Analysis of Results
	I expected the GPU accelerated algorithms to be faster than non GPU, and Sort and sweep to be faster than naive - across the board. However this was not true. When it came to GPU vs non-GPU, when the number of particles was small, like 10, GPU was slower. This is probably because of the memory transfer overhead. Sort and sweep worked best when the reigon was sparse. When the reigon is sparse, the algorithm is able to rule out many unneccesary comparisons. For example, when I put 10000 particles of radius 1 in a 2x2 box, Sort and sweep was actually slower - due to the overhead of calulating means, variance, sorting... 

	Here are some run time comparisons I found interesting:

	# Small number of particles
	10 particles with radii in the range 1 to 1 (inclusive), inside a bounding box with dimemsions 10 by 10
		Computation time for Naive Alg, CPU: 1e-06s
		Computation time for Optimized Alg 1 (Sort and Sweep), CPU: 1.7e-05s
		Computation time for Naive Alg, GPU: 3.9e-05s
		Computation time for Optimized Alg 1 (Sort and Sweep), GPU: 0.000554s (SLOWEST)

	# Sparse Reigon
	10000 particles with radii in the range 1 to 2 (inclusive), inside a bounding box with dimemsions 100000 by 100000
		Computation time for Naive Alg, CPU: 0.157667s
		Computation time for Optimized Alg 1 (Sort and Sweep), CPU: 0.001778s
		Computation time for Naive Alg, GPU: 0.023811s
		Computation time for Optimized Alg 1 (Sort and Sweep), GPU: 0.00138s (FASTEST)

	# Dense Reigon
	10000 particles with radii in the range 1 to 1 (inclusive), inside a bounding box with dimemsions 3 by 3
		Computation time for Naive Alg, CPU: 0.922125s
		Computation time for Optimized Alg 1 (Sort and Sweep), CPU: 0.6263s
		Computation time for Naive Alg, GPU: 0.043441s (FASTEST)
		Computation time for Optimized Alg 1 (Sort and Sweep), GPU: 0.763119s


Project Background
	This collision detection system will be aimed at video games, not scientific simulations. We make this distinction because of the level of accuracy needed. Ideally, a scientific simulation models the real world as accurately as possible. Speed is not the biggest issue (although it does have to be fast enough to terminate at some point). On the other hand, video games are played in real time so speed takes precedence over accuracy.

	There are 2 main types of simulations: discrete and continuous. Continuous simulations generally use laws of physics to calculate the trajectory of objects, and therefore finding when they collide. Discrete simulations check every time step for a collision. We will be implementing a discrete simulation because there is no way to model a player’s trajectory unless we can read their mind.



Things I learned:
	At first, my naive implementation was super slow - .5s when the CPU version was on the order of .00001s.
	I recorded the times of various segments of my GPU kernel calling code, and found these results:
		malloc takes 0.528577s
		copy takes 0.000028s
		Kernal takes 0.000013s
	I realized that malloc takes up a lot of time. As a side note, when I report the times of 
	execution, I am not including the malloc time. This makes sense from a real world perspective,
	because you would only allocate the space for particles/output once, and just write over it
	after that. 

	Also, I learned to use reductions pretty well because they were required for several parts of the code. 


Pitfalls:
	I ran into many strange bugs while completing this project. For the longest time, my kernel would 
	fail with a 'invalid device function' errorString. I tried many things, eventually running an empty
	kernel and doing NO memory allocations beforehand - and the kernel still failed even though it had 
	nothing in it. Finally, I read a post (https://github.com/BVLC/caffe/issues/138) that told me to change
	my makefile:
		 -gencode arch=compute_20,code=sm_20 
		-gencode arch=compute_20,code=sm_21 
		-gencode arch=compute_30,code=sm_30 
		-gencode arch=compute_35,code=sm_35 
		-gencode arch=compute_50,code=sm_50
	which I did and it solved the problem. 

