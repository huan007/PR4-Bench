/*
 *	Implementation of a barrier using conditonal variables
 *	Author: Huan Nguyen
 *	PID: A12871523
 *	email: hpn007@ucsd.edu
 */
void barrier(pthread_mutex_t *mutex, pthread_cond_t *cond, int* counter, int total, int rank);
