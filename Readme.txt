
To Run:
	In the directory 'src', you can compile with:
		g++ -o collision main.cpp collision_solve_host.cpp
	There is currently no GPU code. 

	Run with 
		/collision [-dg]
		d turns debug mode on
		g graphs the positions of the particles at the end


Things I learned:
	
	1)
	At first, my naive implementation was super slow - .5s when the CPU version was on the order of .00001s.
	I recorded the times of various segments of my GPU kernel calling code, and found these results:
		malloc takes 0.528577s
		copy takes 0.000028s
		Kernal takes 0.000013s
	I realized that malloc takes up a lot of time. As a side note, when I report the times of 
	execution, I am not including the malloc time. This makes sense from a real world perspective,
	because you would only allocate the space for particles/output once, and just write over it
	after that. 


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




TODO::

- used reduction in GPU op 1



- optimized1 gpu: why no kernel