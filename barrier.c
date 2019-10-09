/*
 *	Implementation of a barrier using conditonal variables
 *	Author: Huan Nguyen
 *	email: hpn007@ucsd.edu
 */
#include <pthread.h>
#include <stdio.h>
//Barrier method
void barrier(pthread_mutex_t *mutex, pthread_cond_t *cond, int* counter, int total, int rank)
{

	//Barrier
	pthread_mutex_lock(mutex);
	(*counter)++;
	if (*counter == total) {
		*counter = 0;
		//printf("All threads have arrived! Thread #%d was the last thread\n", rank);
		pthread_cond_broadcast(cond);
	}
	
	else
	{
		//printf("Thread #%d has arrived\n", rank);
		while (pthread_cond_wait(cond, mutex) != 0);
		//Assuming that after condition, the thread will get mutex back
	}
	pthread_mutex_unlock(mutex);

}
