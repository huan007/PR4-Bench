SOURCES = heat2d.c heat2d_solver.c 
CC = gcc
CFLAGS = -g

default: heat2d 

heat2d_solver.o: heat2d_solver.c heat2d_solver.h 
	$(CC)  $(CFLAGS) -c heat2d_solver.c 

heat2d: heat2d_solver.o heat2d.c
	$(CC)  $(CFLAGS) -o heat2d heat2d.c heat2d_solver.o 

hello: heat2dPara.c barrier.c heat2d_solver.o
	$(CC) -g  -o hello heat2dPara.c heat2d_solver.c barrier.c -lpthread -lm

runbar: barrierTest.c
	$(CC) -o barrier barrierTest.c barrier.c -lpthread

clean:
	-/bin/rm *o heat2d
