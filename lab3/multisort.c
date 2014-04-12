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
		// Una tasca de tareador por cada llamada


                // Base case
#if _EXTRAE_
                Extrae_event(PROGRAM, MERGE);
#endif
#if _TAREADOR_
		tareador_start_task("basemerge");
#endif
                basicmerge(n, left, right, result, start, length);
#if _TAREADOR_
		tareador_end_task();
#endif
#if _EXTRAE_
                Extrae_event(PROGRAM, END);
#endif
        } else {
                // Recursive decomposition
#if _TAREADOR_
		tareador_start_task("mergeinside1");
#endif
                merge(n, left, right, result, start, length/2);
#if _TAREADOR_
		tareador_end_task();
		tareador_start_task("mergeinside2");
#endif
                merge(n, left, right, result, start + length/2, length/2);
#if _TAREADOR_
		tareador_end_task();
#endif
        }
}

void multisort(long n, T data[n], T tmp[n]) {       
 if (n >= MIN_SORT_SIZE*4L) {
                // Recursive decomposition

		// Una tasca de tareador por cada llamada
#if _TAREADOR_
		tareador_start_task("multisort1");
#endif
                multisort(n/4L, &data[0], &tmp[0]);
#if _TAREADOR_
		tareador_end_task();
		tareador_start_task("multisort2");
#endif
                multisort(n/4L, &data[n/4L], &tmp[n/4L]);
#if _TAREADOR_
		tareador_end_task();
		tareador_start_task("multisort3");
#endif
                multisort(n/4L, &data[n/2L], &tmp[n/2L]);

#if _TAREADOR_
		tareador_end_task();
		tareador_start_task("multisort4");
#endif
                multisort(n/4L, &data[3L*n/4L], &tmp[3L*n/4L]);
#if _TAREADOR_
		tareador_end_task();
		tareador_start_task("merge1");
#endif
                merge(n/4L, &data[0], &data[n/4L], &tmp[0], 0, n/2L);

#if _TAREADOR_
                tareador_end_task();
                tareador_start_task("merge2");    
#endif
                merge(n/4L, &data[n/2L], &data[3L*n/4L], &tmp[n/2L], 0, n/2L);
#if _TAREADOR_
                tareador_end_task();
                tareador_start_task("merge3");    
#endif
                merge(n/2L, &tmp[0], &tmp[n/2L], &data[0], 0, n);
#if _TAREADOR_
                tareador_end_task();    
#endif

	} else {
		// Base case
#if _EXTRAE_
		Extrae_event(PROGRAM, SORT);
#endif
#if _TAREADOR_
		tareador_start_task("basesort");
#endif
		basicsort(n, data);
#if _TAREADOR_
		tareador_end_task();
#endif
#if _EXTRAE_
		Extrae_event(PROGRAM, END);
#endif
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
