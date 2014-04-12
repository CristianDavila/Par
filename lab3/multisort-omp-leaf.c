#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#if _EXTRAE_
#include "extrae_user_events.h"
#endif
#if _TAREADOR_
#include <tareador_hooks.h>
#endif

// N and MIN must be powers of 2
long N;
long MIN_SORT_SIZE;
long MIN_MERGE_SIZE;

#define T int

#if _EXTRAE_
// Extrae constants
#define PROGRAM		1000
#define END		0
#define SORT		1
#define MERGE		2
#define MULTISORT	3
#define INITIALIZE	4
#define CHECK		5
#endif 

void basicsort(long n, T data[n]);

void basicmerge(long n, T left[n], T right[n], T result[n*2], long start, long length);

void merge(long n, T left[n], T right[n], T result[n*2], long start, long length) {
        if (length < MIN_MERGE_SIZE*2L) {
            // Base case
			#pragma omp task
			{
				#if _EXTRAE_
						Extrae_event(PROGRAM, MERGE);
				#endif
						basicmerge(n, left, right, result, start, length);
				#if _EXTRAE_
						Extrae_event(PROGRAM, END);
				#endif
			}
        } else {
                // Recursive decomposition

		merge(n, left, right, result, start, length/2);

		merge(n, left, right, result, start + length/2, length/2);

        }
}

void multisort(long n, T data[n], T tmp[n]) {       
 if (n >= MIN_SORT_SIZE*4L) {
                // Recursive decomposition

		multisort(n/4L, &data[0], &tmp[0]);

		multisort(n/4L, &data[n/4L], &tmp[n/4L]);

		multisort(n/4L, &data[n/2L], &tmp[n/2L]);

		multisort(n/4L, &data[3L*n/4L], &tmp[3L*n/4L]);

		#pragma omp taskwait
		merge(n/4L, &data[0], &data[n/4L], &tmp[0], 0, n/2L);

		merge(n/4L, &data[n/2L], &data[3L*n/4L], &tmp[n/2L], 0, n/2L);
		#pragma omp taskwait

		merge(n/2L, &tmp[0], &tmp[n/2L], &data[0], 0, n);
	} else {
		// Base case
		#pragma omp task
		{
			#if _EXTRAE_
					Extrae_event(PROGRAM, SORT);
			#endif
					basicsort(n, data);
			#if _EXTRAE_
					Extrae_event(PROGRAM, END);
			#endif
		}
	}
}

static void initialize(long length, T data[length]) {
   long i;
   for (i = 0; i < length; i++) {
      if (i==0) {
         data[i] = rand();
      } else {
         data[i] = ((data[i-1]+1) * i * 104723L) % N;
      }
   }
}

static void clear(long length, T data[length]) {
   long i;
   for (i = 0; i < length; i++) {
      data[i] = 0;
   }
}

void check_sorted(long n, T data[n]) 
{
   int unsorted=0;
   for (int i=1; i<n; i++)
      if (data[i-1] > data[i]) unsorted++;
   if (unsorted > 0)
      printf ("\nERROR: data is NOT properly sorted. There are %d unordered positions\n\n",unsorted);
   else {
//      printf ("data IS ordered; ");
   }
}

void do_sort(long n, T data[n], T tmp[n]) { 

#if _EXTRAE_
   Extrae_event(PROGRAM, MULTISORT);
#else
   double sort_time = omp_get_wtime();
#endif
	#pragma omp parallel
	#pragma omp single
	multisort(N, data, tmp);

#if _EXTRAE_
   Extrae_event(PROGRAM,END);
#else
   sort_time = omp_get_wtime() - sort_time;
   fprintf(stdout, "%g\n", sort_time);
#endif

#if _EXTRAE_
   Extrae_event(PROGRAM,CHECK);
#endif
   check_sorted (N, data);
#if _EXTRAE_
   Extrae_event(PROGRAM,END);
#endif
}

int main(int argc, char **argv) {

        if (argc != 4) {
                fprintf(stderr, "Usage: %s <vector size in K> <sort size in K> <merge size in K>\n", argv[0]);
                return 1;
        }

#if _EXTRAE_
	Extrae_init();
#endif
	N = atol(argv[1]) * 1024L;
	MIN_SORT_SIZE = atol(argv[2]) * 1024L;
        MIN_MERGE_SIZE = atol(argv[3]) * 1024L;
	
	T *data = malloc(N*sizeof(T));
	T *tmp = malloc(N*sizeof(T));
	
#if _TAREADOR_
	tareador_ON();
#endif

#if _EXTRAE_
	Extrae_event(PROGRAM, INITIALIZE);
#else
	double init_time = omp_get_wtime();
#endif
	initialize(N, data);
	clear(N, tmp);
#if _EXTRAE_
	Extrae_event(PROGRAM, END);
#else
	init_time = omp_get_wtime() - init_time;
    	fprintf(stdout, "Initialization time in seconds = %g\n", init_time);
#endif

    	fprintf(stdout, "Multisort execution time using randomly generated data = ");
   	do_sort(N, data, tmp); //sort randomly generated data 
	
#if _TAREADOR_
	tareador_OFF();
#endif

#if _EXTRAE_
	Extrae_event(PROGRAM, INITIALIZE);
	Extrae_event(PROGRAM, END);
#endif
    	fprintf(stdout, "Multisort execution time using already sorted data = ");
   	do_sort(N, data, tmp); // sort already sorted

#if _EXTRAE_
	Extrae_event(PROGRAM, INITIALIZE);
#endif
   	for (int i=0; i<N/2; i++) { // Reverse order
      		double tmp =data[N-1-i];
      		data[N-1-i] = data[i];
      		data[i]=tmp;
   	}
#if _EXTRAE_
	Extrae_event(PROGRAM, END);
#endif
    	fprintf(stdout, "Multisort execution time using reverse order data = ");
   	do_sort(N, data, tmp); //sort data in inverted order

#if _EXTRAE_
	Extrae_fini();
#endif
    	fprintf(stdout, "Multisort program finished\n");
	return 0;
}
