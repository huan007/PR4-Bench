/*
 *	Pthread version of heatmap 2D
 *	Author: Huan Nguyen
 *	PID: A12871523
 *	email: hpn007@ucsd.edu
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
//#include "heat2d_solver.h" 
#include "barrier.h"

#define TOP 0 
#define MID 1
#define BOT 2
#define WHOLE 3

typedef struct {
	//Extra variables
	int rank;
	int position;
	int copyStart;
	int copyEnd;
	//Variables needed for heat2dsolve
	int M;
	double eps;
	int print;
	double** u;
	double* tol;
	int* iter;
} Param;

/* Global variable: accessible to all threads */
int thread_count;
int globalM;
int globalN;
double** u;
Param* paramList;
int* itersList;
double* tolList;

//Barrier's variables
pthread_mutex_t mutex;
pthread_cond_t cond;
int counter;
int t_count;

//State variable
double globalDiff;
pthread_mutex_t mutex_eps;
pthread_mutex_t mutex_print;

void *Hello(void* rank);	//thread fucntion
void *solve(void* param);
double cpu_time ( void );
void initialize_plate(int M, int N, double Tl, double Tr, 
		double Tt, double Tb, double **u);
void print(double **u, int M, int N);


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


	//Parse number of threads if possible
	if (argc > 9)
		thread_count = strtol(argv[9], NULL, 10);
	else
		thread_count = 1;

	//error checking
	if (globalM < 0 || globalN < 0 || thread_count < 0 || eps < 0)
		usage();

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

	//Initialize parameter list
	paramList = malloc(thread_count * sizeof(Param));
	itersList = malloc(thread_count * sizeof(int));
	tolList = malloc(thread_count * sizeof(double));

	//Set globalDiff
	globalDiff = 2.0 * eps;

	printf("Initializing grid...");
	/* Set the boundary values, which don't change.  */
	initialize_plate(globalM,globalN,Tl,Tr,Tt,Tb,u);
	printf(" Done!\n");

	//Creating threads
	thread_handles = malloc (thread_count * sizeof(pthread_t));
	int iters = 0;
	double tol = 0;
	counter = 0;


	//Temporary variables to divide work
	int start = 0;
	int step = (int) ceil( (double) globalM / (double) thread_count);
	int end = 0;

	//print(u, globalM, globalN);

	ctime1 = cpu_time ( );
	for (thread = 0; thread < thread_count; thread++)
	{
		int threadM = 0;
		double **threadU;
		//Create Param to pass in
		Param* param = &(paramList[thread]);
		param->rank = thread;

		//Determine the position
		start = end;
		end += step;	//Increment end
		if (thread_count == 1)		//If there are only one thread
		{
			param->position = WHOLE;
			threadU = u;
			threadM = globalM;
			start = 0;
			end = globalM;
		}

		if (start == 0 && thread_count > 1)
		{
			param->position = TOP;
			threadM++;		//Add one buffer row at the bottom
			threadM += end - start;		//Calculate final size after adding buffers
			threadU = malloc(threadM * sizeof(double *));

			//Map u to threadU
			for (i = 0, j = start; j < end; i++, j++)
				threadU[i] = u[j];
			//Create additional buffer row at the bottom
			threadU[i] = (double *) malloc(globalN  * sizeof(double));

		}


		//if we went pass the end, then this is the bottom piece
		if (start != 0 && end >= globalM)
		{
			end = globalM;
			param->position = BOT;
			threadM++;		//Add one buffer row at the top
			threadM += end - start;		//Calculate final size after adding buffers
			threadU = malloc(threadM * sizeof(double *));

			//Create additional buffer row at the top
			threadU[0] = (double *) malloc(globalN  * sizeof(double));
			//Map u to threadU
			for (i = 1, j = start; j < end; i++, j++)
				threadU[i] = u[j];

		}
		//Middle pieces
		else if (start != 0)
		{
			param->position = MID;
			threadM+=2;		//Add two buffer rows both on top and bottom
			threadM += end - start;		//Calculate final size after adding buffers
			threadU = malloc(threadM * sizeof(double *));

			//Create additional buffer row at the top
			threadU[0] = (double *) malloc(globalN  * sizeof(double));
			//Map u to threadU
			for (i = 1, j = start; j < end; i++, j++)
				threadU[i] = u[j];
			//Create additional buffer row at the bottom
			threadU[i] = (double *) malloc(globalN  * sizeof(double));
		}



		param-> copyStart = start;
		param-> copyEnd = end;
		printf("\n");

		//printf("Thread #%d\tPosition %d\tSize: %d\tStart :%d\tEnd: %d\n",
		//	thread, param -> position, threadM, param -> copyStart, param -> copyEnd);
		//print(threadU, threadM, globalN);

		//Variables needed for solve
		param->M = threadM;
		param->eps = eps;
		param->print = 1;
		param->u = threadU;
		param->tol = &(tolList[thread]);
		param->iter = &(itersList[thread]);
		pthread_create(&thread_handles[thread], NULL, solve, (void*) param);
	}


	//Joining threads
	for (thread = 0; thread < thread_count; thread++)
		pthread_join(thread_handles[thread], NULL);

	ctime2 = cpu_time ( );
	ctime = ctime2 - ctime1;

	printf ( "\n  %8d  %f\n", iters, tol );
	printf ( "\n  Error tolerance achieved.\n" );
	printf ( "  CPU time = %f\n", ctime );



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

	//Free buffers and pointer maps
	for (thread = 0; thread < thread_count; thread++)
	{
		printf("--------Free thread #%ld---------\n", thread);
		int size = paramList[thread].M;
		printf("Position: %d\t Size: %d\n", paramList[thread].position, paramList[thread].M);
		//Free top buffer
		if (paramList[thread].position == TOP || paramList[thread].position == MID)
			free(paramList[thread].u[size-1]);
		//Free bottom buffer
		if (paramList[thread].position == BOT || paramList[thread].position == MID)
			free(paramList[thread].u[0]);

		printf("Freeing threadU\n");
		//Free pointer map
		if (paramList[thread].position != WHOLE)
			free(paramList[thread].u);
	}

	//Free the rest of memory
	printf("\n");
	printf("Free paramList\n");
	free(paramList);
	printf("Free itersList\n");
	free(itersList);
	printf("Free tolsList\n");
	free(tolList);
	free(thread_handles);
	printf("Done\n");

	return 0;

	//printf("Hello from main thread\n");
}

/* Modified version of heat2dSolve that utilize multiple threads
 *	Parameters: M x N matrix with its sizes and epsilon values. Also receive
 *	positin of the block (where does the block fit into the big picture). Also
 *	has the starting and ending coordinate of the block (rows)
 *	Return: number of iterations that it took
 */
int heat2dSolvePara(int M, int N, double eps, int printBool, double **threadU, double *tol,
		int rank, int position, int copyStart, int copyEnd)
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

	pthread_mutex_lock(&mutex_print);
	if (printBool && rank == 0) 
		printf( "\n Iteration  Change\n" );
	pthread_mutex_unlock(&mutex_print);

	while ( eps <= globalDiff )
	{
		/*
		 * 	Copy phrase, no one write anything
		 */
		if (position != WHOLE)
		{
			//Copy bottom's buffer
			if (position == TOP || position == MID)
			{
				//printf("Rank %d is coping bottom\n", rank);
				memcpy(threadU[M-1], u[copyEnd], N*sizeof(double));
				for (i = 0; i < N; i++)
					threadU[M-1][i] = u[copyEnd][i];
			}
			//Copy top's buffer
			if (position == BOT || position == MID)
			{
				//printf("Rank %d is coping top\n", rank);
				memcpy(threadU[0], u[copyStart-1], N*sizeof(double));
				for (i = 0; i < N; i++)
					threadU[0][i] = u[copyStart-1][i];
			}
			//Wait for everyone to finish copying before move on to calculation
			//phase
		}
		//Copy the first row into rowCurr
		memcpy(rowCurr, threadU[0], N*sizeof(double));
		//pthread_mutex_lock(&mutex_print);
		//printf("-------------Rank %d -----Iteration %d------------\n", rank, iterations);
		//print(threadU, M, N);
		//pthread_mutex_unlock(&mutex_print);

		//Make sure that everyone is ready (copied bottom buffer and first row)
		barrier(&mutex, &cond, &counter, thread_count, rank);
		//Reset globalDiff
		if (rank == 0)
			globalDiff = 0;
		barrier(&mutex, &cond, &counter, thread_count, rank);
		/*
		   Determine the new estimate of the solution at the interior points.
		   The new solution W is the average of north, south, east and west 
		   neighbors.  */
		diff = 0.0;
		for ( i = 1; i < M - 1; i++ )
		{
			/* swap rowPrev and rowCurr pointers. Save the current row */
			rowTmp = rowPrev; rowPrev=rowCurr; rowCurr=rowTmp;
			memcpy(rowCurr, threadU[i], N*sizeof(double));
			for ( j = 1; j < N - 1; j++ )
			{
				threadU[i][j] = (rowPrev[j] + threadU[i+1][j] + 
						rowCurr[j-1] + rowCurr[j+1] ) / 4.0;

				double delta = fabs(rowCurr[j] - threadU[i][j]);
				if ( diff < delta ) 
				{
					diff = delta; 
				}
			}
		}
		iterations++;
		//Update global differences
		pthread_mutex_lock(&mutex_eps);
		if (diff > globalDiff)
		{
			globalDiff = diff;
		}
		pthread_mutex_unlock(&mutex_eps);

		if ( printBool && iterations == iterations_print )
		{
			if (rank == 0)
				printf ( "  %8d  %f\n", iterations, diff );
			iterations_print *= 2;
		}
		barrier(&mutex, &cond, &counter, thread_count, rank);
	} 
	/* memory cleanup */
	free(rowCurr);
	free(rowPrev);
	*tol = diff;
	return iterations;
}
/* 
 *	Dummy method for starting threads. This method will parse all the parameters
 *	passed into the thread and pass it onto heat2dSolvePara()
 */
void *solve(void* param)
{
	//printf("Parsing parameters...\n");
	Param* values = (Param*) param;
	//Extra variables
	int rank = values->rank;
	int position = values->position;
	int copyStart = values->copyStart;
	int copyEnd = values->copyEnd;
	int M = values->M;
	int N = globalN;
	double eps = values->eps;
	int print = values->print;
	double** u = values->u;
	double* tol = values->tol;
	//printf("Finished parshing parameters\n");

	//printf("M: %d\nN: %d\neps: %f\nprint: %d\nu: %p\ntol: %p\n", M, N, eps, print, u, tol);
	heat2dSolvePara(M, N, eps, print, u, tol, rank, position, copyStart, copyEnd);

}


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
/******************************************************************************/
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
	 * 	Initialize the interior solution to the mean value.
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

/*
 * Print out the specified grid
 * Paramters: Pointer to the grid with its sizes
 */
void print(double **u, int M, int N)
{
	int i, j;
	for (i = 0; i < M; i++)
	{
		for (j = 0; j < N; j++)
		{
			printf("%6.8f ", u[i][j]);
		}
		printf("\n");
	}
}
