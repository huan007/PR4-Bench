/*
 *	Tester for barrier
 *	Author: Huan Nguyen
 *	email: hpn007@ucsd.edu
 */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "barrier.h"

void *thread_work (void* rank);

pthread_mutex_t mutex;
pthread_cond_t cond;
int counter;
int t_count;
int i_count;

int main (int argc, char* argv[])
{
	if (argc < 3)
		exit(-1);

	t_count = atoi(argv[1]);
	i_count = atoi(argv[2]);
	counter = 0;
	
	//Allcocate thread's handles
	pthread_t* handles = malloc (t_count * sizeof(pthread_t));

	int thread = 0;
	//Spin up T threads
	for (thread = 0; thread < t_count; thread++)
		pthread_create(&handles[thread], NULL, thread_work,  (void *) thread);

	//Join threads
	for (thread = 0; thread < t_count; thread++)
		pthread_join(handles[thread], NULL);
}

void *thread_work (void* rank)
{
	int my_rank = (int) rank;
	
	int i = 0;
	for (i = 0; i < i_count; i++)
	{
		//printf("(%d)\tThread #%d is entering barrier...\n", i, my_rank);
		barrier(&mutex, &cond, &counter, t_count);
	}
}
