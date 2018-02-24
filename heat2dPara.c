#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
//#include "heat2d_solver.h" 
#include "barrier.h"

/* Global variable: accessible to all threads */
int thread_count;
int globalM;
int globalN;
double** u;

//Barrier's variables
pthread_mutex_t mutex;
pthread_cond_t cond;
int counter;
int t_count;

//State variable
double globalEps;
pthread_mutex_t* mutex_row;
pthread_mutex_t mutex_eps;

void *Hello(void* rank);	//thread fucntion
void *solve(void* param);
double cpu_time ( void );
void initialize_plate(int M, int N, double Tl, double Tr, 
		double Tt, double Tb, double **u);

typedef struct {
	int M;
	int N;
	double eps;
	int print;
	double** u;
	double* tol;
	int* iter;
} Param;

int usage()
{
	fprintf(stderr, "usage: heat2d M N Tl Tr Tt Tb eps file\n");
	exit(-1);
}

int main(int argc, char* argv[])
{
	long thread;
	pthread_t* thread_handles;
	double Tl,Tr, Tt, Tb;
	double eps = 0;
	char *output_file;
	FILE* fp;

	int i, j;
	double ctime, ctime1, ctime2;

	//Parse inputs - Remember to check validity
	if (argc < 9) usage();
	globalM = atoi(argv[1]);
	globalN = atoi(argv[2]);
	Tl = atof(argv[3]);
	Tr = atof(argv[4]);
	Tt = atof(argv[5]);
	Tb = atof(argv[6]);
	eps = atof(argv[7]);
	output_file = argv[8];
	
	if (argc > 9)
		thread_count = strtol(argv[9], NULL, 10);
	else
		thread_count = 1;

	printf ( "HEAT2D\n" );
	printf ( "  C version\n" );
	printf ( "  A program to solve for the steady state temperature distribution\n" );
	printf ( "  over a rectangular plate.\n" );
	printf ( "  Spatial grid of %d by %d points.\n", globalM, globalN );
	printf ( "\n" );

	u = (double **) malloc(globalM*sizeof(double *));
	for (i = 0; i < globalM; i ++) {
		u[i] = (double *) malloc(globalN  * sizeof(double));
	}

	printf("Initializing grid...");
	/* Set the boundary values, which don't change.  */
	initialize_plate(globalM,globalN,Tl,Tr,Tt,Tb,u);
	printf(" Done!\n");
	
	//Creating threads
	thread_handles = malloc (thread_count * sizeof(pthread_t));
	int iters = 0;
	double tol = 0;
	counter = 0;
	mutex_row = malloc (globalM * sizeof(pthread_mutex_t));

	printf("Creating threads...");
	ctime1 = cpu_time ( );
	for (thread = 0; thread < thread_count; thread++)
	{
		//Create Param to pass in
		Param param;
		param.M = globalM;
		param.N = globalN;
		param.eps = eps;
		param.print = 1;
		param.u = u;
		param.tol = &tol;
		param.iter = &iters;
		pthread_create(&thread_handles[thread], NULL, solve, (void*) &param);
		printf("Spawned thread %d\n", thread);
	}

	ctime2 = cpu_time ( );
	ctime = ctime2 - ctime1;
	
	//Joining threads
	for (thread = 0; thread < thread_count; thread++)
		pthread_join(thread_handles[thread], NULL);

	printf ( "\n  %8d  %f\n", iters, tol );
	printf ( "\n  Error tolerance achieved.\n" );
	printf ( "  CPU time = %f\n", ctime );
	

	free(thread_handles);

	/* Write the solution to the output file.  */
	fp = fopen ( output_file, "w" );

	fprintf ( fp, "%d\n", globalM );
	fprintf ( fp, "%d\n", globalN );

	for ( i = 0; i < globalM; i++ )
	{
		for ( j = 0; j < globalN; j++)
		{
			fprintf ( fp, "%15.7f ", u[i][j] );
		}
		fputc ( '\n', fp);
	}
	fclose ( fp );

	printf ( "\n" );
	printf ("  Solution written to the output file '%s'\n", output_file );

	/* All done!  */
	printf ( "\n" );
	printf ( "HEAT2D:\n" );
	printf ( "  Normal end of execution.\n" );

	for (i = 0; i < globalM; i++)
		free(u[i]);
	free(u);
	return 0;

	//printf("Hello from main thread\n");
}

int heat2dSolvePara(int M, int N, double eps, int print, double **u, double *tol)
{

	int iterations = 0;
	int iterations_print = 1;
	int i,j;
	double diff = 2.0 * eps;
	double *rowPrev; /* copy of the previous row in u */
	double *rowCurr; /* copy of the current row in in */
	double *rowTmp;

	rowPrev = calloc(N, sizeof(double));
	rowCurr = calloc(N, sizeof(double));
	if (print) 
		printf( "\n Iteration  Change\n" );

	while ( eps <= diff )
	{
		/*
			Initialize copy of "current" row 
		*/
		pthread_mutex_lock(&(mutex_row[0]));
		memcpy(rowCurr, u[0], N*sizeof(double));
		pthread_mutex_unlock(&(mutex_row[0]));
		barrier(&mutex, &cond, &counter, thread_count);
		/*
		Determine the new estimate of the solution at the interior points.
		The new solution W is the average of north, south, east and west 
		neighbors.  */
		diff = 0.0;
		for ( i = 1; i < M - 1; i++ )
		{
			/* swap rowPrev and rowCurr pointers. Save the current row */
			rowTmp = rowPrev; rowPrev=rowCurr; rowCurr=rowTmp;
			pthread_mutex_lock(&(mutex_row[i]));
			memcpy(rowCurr, u[i], N*sizeof(double));
			pthread_mutex_unlock(&(mutex_row[i]));
			barrier(&mutex, &cond, &counter, thread_count);
	
			pthread_mutex_lock(&(mutex_row[i]));
			for ( j = 1; j < N - 1; j++ )
			{
				u[i][j] = (rowPrev[j] + u[i+1][j] + 
					rowCurr[j-1] + rowCurr[j+1] ) / 4.0;
	
				double delta = fabs(rowCurr[j] - u[i][j]);
				if ( diff < delta ) 
				{
					diff = delta; 
				}
			}
			pthread_mutex_unlock(&(mutex_row[i]));
		}
		iterations++;
		//Update global differences
		pthread_mutex_lock(&mutex_eps);
		if (diff < globalEps)
			globalEps = diff;
		pthread_mutex_unlock(&mutex_eps);

		if ( print && iterations == iterations_print )
		{
			printf ( "  %8d  %f\n", iterations, diff );
			iterations_print *= 2;
		}
		barrier(&mutex, &cond, &counter, thread_count);
	} 
	/* memory cleanup */
	free(rowCurr);
	free(rowPrev);
	*tol = diff;
	return iterations;
}

void *Hello(void* rank)
{
	long my_rank = (long) rank;

	printf("Hello from thread %ld of %d\n", my_rank, thread_count);

	return NULL;
}

void *solve(void* param)
{
	printf("Parsing parameters...\n");
	Param* values = (Param*) param;
	int M = values->M;
	int N = values->N;
	double eps = values->eps;
	int print = values->print;
	double** u = values->u;
	double* tol = values->tol;
	printf("Finished parshing parameters\n");

	printf("M: %d\nN: %d\neps: %f\nprint: %d\nu: %p\ntol: %p\n", M, N, eps, print, u, tol);
	heat2dSolvePara(M, N, eps, print, u, tol);
	
	//int heat2dSolve(int M, int N, double eps, int print, double **u, double *tol)
}


/******************************************************************************/
/******************************************************************************/
/*
Purpose:
	CPU_TIME returns the current reading on the CPU clock.
Licensing:
	This code is distributed under the GNU LGPL license. 
Modified:
	06 June 2005
Author:
	John Burkardt
Parameters:
	Output, double CPU_TIME, the current reading of the CPU clock, in seconds.
*/
double cpu_time ( void )
{
	double value;
	value = ( double ) clock ( ) / ( double ) CLOCKS_PER_SEC;
	return value;
}

/******************************************************************************/
/* Initialize the plate with boundary and mean temperature */
/******************************************************************************/
void initialize_plate(int M, int N, double Tl, double Tr, double Tt,
		double Tb, double **u)
{
	int i, j;
	for ( i = 1; i < M - 1; i++ )
	{
		u[i][0] = Tl;
		u[i][N-1] = Tr;
	}
	for ( j = 0; j < N; j++ )
	{
		u[0][j] = Tt;
		u[M-1][j] = Tb;
	}
	/*
	   Average the boundary values, to come up with a reasonable
	   initial value for the interior.
	   */
	double mean = 0.0;
	for ( i = 1; i < M - 1; i++ )
	{
		mean += u[i][0];
		mean += u[i][N-1];
	}
	for ( j = 0; j < N; j++ )
	{
		mean += u[0][j];
		mean += u[M-1][j];
	}
	mean = mean / ( double ) ( 2 * M + 2 * N - 4 );
	/* 
	   Initialize the interior solution to the mean value.
	   */
	for ( i = 1; i < M - 1; i++ )
	{
		for ( j = 1; j < N - 1; j++ )
		{
			u[i][j] = mean;
		}
	}
	return;
}


