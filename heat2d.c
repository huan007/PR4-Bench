# include <stdlib.h>
# include <stdio.h>
# include <math.h>
# include <time.h>
# include "heat2d_solver.h" 

double cpu_time ( void );
void initialize_plate(int M, int N, double Tl, double Tr, 
		double Tt, double Tb, double **u);

/******************************************************************************/

int usage()
{
	fprintf(stderr, "usage: heat2d M N Tl Tr Tt Tb eps file\n");
	exit(-1);
}


/******************************************************************************/
/*
Purpose:
	MAIN is the main program for HEATED_PLATE.
Discussion:
	This code solves the steady state heat equation on a rectangular region.

	The physical region, and the boundary conditions, are suggested
	by this diagram;

	    		U = Tt
			+------------------+
			|                  |
	U = Tl	|                  | U = Tr 
			|                  |
			+------------------+
			    U = Tb

	The region is covered with a grid of M by N nodes, and an N by N
	array U is used to record the temperature.  

	The steady state solution to the discrete heat equation satisfies the
	following condition at an interior grid point:

	U[Central] = (1/4) * ( U[North] + U[South] + U[East] + U[Uest] )

	where "Central" is the index of the grid point, "North" is the index
	of its immediate neighbor to the "north", and so on.

	Given an approximate solution of the steady state heat equation, a
	"better" solution is given by replacing each interior point by the
	average of its 4 neighbors - in other words, by using the condition
	as an ASSIGNMENT statement:

	U[Central]  <=  (1/4) * ( U[North] + U[South] + U[East] + U[Uest] )

	If this process is repeated often enough, the difference 
	between successive estimates of the solution will go to zero.

	This program carries out such an iteration, using a tolerance specified by
	the user, and writes the final estimate of the solution to a file that can
	be used for graphic processing.

Licensing:
	This code is distributed under the GNU LGPL license. 
Modified:
	22 July 2008
Author:
	Original C version by Michael Quinn.
	Modifications by John Burkardt.
	More modifications by Philip Papadopoulos
Reference:
	Michael Quinn,
	Parallel Programming in C with MPI and OpenMP,
	McGraw-Hill, 2004,
	ISBN13: 978-0071232654,
	LC: QA76.73.C15.Q55.

Parameters:
	Commandline argument 1,  M  number of rows
	Commandline argument 2,  N  number of columns 
	Commandline argument 3, double Tleft, T along the left boundary 
	Commandline argument 4, double Tright, T along the right boundary.  
	Commandline argument 5, double Ttop, T along the top boundary.  
	Commandline argument 6, double Tbottom, T along the bottom boundary.  
	Commandline argument 7, double EPSILON, the error tolerance.  
	Commandline argument 8, char *OUTPUT_FILE, the name of the file into which
	the steady state solution is written when the program has completed.
*/
int main ( int argc, char *argv[] )
{
	double ctime;
	double ctime1;
	double ctime2;
	double eps;
	FILE *fp;
	int M;
	int N;
	int i,j;
	char *output_file;
	double **u;
	double Tl,Tr,Tt,Tb;

	if (argc < 8) usage();
	M = atoi(argv[1]);
	N = atoi(argv[2]);
	Tl = atof(argv[3]);
	Tr = atof(argv[4]);
	Tt = atof(argv[5]);
	Tb = atof(argv[6]);
	eps = atof(argv[7]);
	output_file = argv[8];

	printf ( "HEAT2D\n" );
	printf ( "  C version\n" );
	printf ( "  A program to solve for the steady state temperature distribution\n" );
	printf ( "  over a rectangular plate.\n" );
	printf ( "  Spatial grid of %d by %d points.\n", M, N );
	printf ( "\n" );

	u = (double **) malloc(M*sizeof(double *));
	for (i = 0; i < M; i ++) {
		u[i] = (double *) malloc(N  * sizeof(double));
	}
	/** Note: u[i][j] = *( *(arr +i) + j) **/
	printf ( "  The iteration will be repeated until the change is <= %G\n", eps );
	printf ( "  Boundary Temperatures  left: %G  right: %G\n", Tl, Tr );
	printf ( "  The steady state solution will be written to '%s'.\n", output_file );

	/* Set the boundary values, which don't change.  */
	initialize_plate(M,N,Tl,Tr,Tb,Tt,u);
	ctime1 = cpu_time ( );
	int iters;
	double tol;
	iters = heat2dSolve(M, N, eps, 1, u, &tol);
	ctime2 = cpu_time ( );
	ctime = ctime2 - ctime1;

	printf ( "\n  %8d  %f\n", iters, tol );
	printf ( "\n  Error tolerance achieved.\n" );
	printf ( "  CPU time = %f\n", ctime );

	/* Write the solution to the output file.  */
	fp = fopen ( output_file, "w" );

	fprintf ( fp, "%d\n", M );
	fprintf ( fp, "%d\n", N );

	for ( i = 0; i < M; i++ )
	{
		for ( j = 0; j < N; j++)
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

	for (i = 0; i < M; i++)
		free(u[i]);
	free(u);
	return 0;
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
