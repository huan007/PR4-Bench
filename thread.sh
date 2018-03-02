#! /bin/bash
for i in {2..12..1}
do
	(time ./heat2d 2000 2000 100 100 50 69 0.001 thread-$i $i) &> time-$i
done
