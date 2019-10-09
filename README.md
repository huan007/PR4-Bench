CSE 160 PR4 - Heat Map Simulation
=========

Description
-----


Install & Requirements
-----
In order to compile this project, you need to install pthread.
To Install pthread in Ubuntu run the following command:
```
sudo apt-get install libpthread-stubs0-dev
```
Then run Make to compile the file.
```
make
```

How to run the program
-----
Run the following command to start computing:
```
./heat2d 2000 2000 100 10 50 50 0.0005 heat2d2K.log 4
```
To Visualize the heat map, use heatmap.py
```
./heatmap.py heat2d2K.log
```

Semantics of the program
-----
**heat2d_solver.c** is the serial verion of the program (only one thread).

**heat2dPara.c** is the multi-threaded version of the program.

Within the ```main``` method of **heat2dPara.c** is where setup and division of data are taken care of. Each thread spawned will run ```heat2dSolvePara``` function to compute. Each iteration threads are synchronized and will update data before every thread start computing again.

Benchmark results
-----
Once the project was completed, I wrote some scripts to benchmark the impact of multhreaded architecture. The benchmark was run on an i7-3770 with 4 physical cores and 8 threads. The map inputs ranged from small (200x200) to large (4k x 4k) maps. It was great to see that my algorithm was able to achieve 70% efficiency when running on 4 threads. In another separate benchmark, the program was run at fixed map size but the number of threads ranging from 1 to 12 threads. The result showed that we receive the most benefit going from 1 thread to 4 threads. Anything after 8 threads we will start to see diminishing returns, or maybe even penalty. This is because we are spawning more threads than the CPU can handle at once, therefore caused cache thrasing and negatively affect the performance.
