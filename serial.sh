#! /bin/bash
for i in {200..4000..200}
do
	(time ./heat2dSerial $i $i 100 100 50 69 0.001 serial-$i) &> time-$i
done
