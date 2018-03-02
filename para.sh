#! /bin/bash
for i in {200..4000..200}
do
	(time ./heat2d $i $i 100 100 50 69 0.001 para-$i 4) &> time-$i
done
