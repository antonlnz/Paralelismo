#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <math.h>

#ifndef DEBUG
#define DEBUG 0
#endif

/* Translation of the DNA bases
   A -> 0
   C -> 1
   G -> 2
   T -> 3
   N -> 4*/

#define M  1000000 // Number of sequences
#define N  200  // Number of bases per sequence

unsigned int g_seed = 0;

int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16) % 5;
}

// The distance between two bases
int base_distance(int base1, int base2){

    if((base1 == 4) || (base2 == 4)){
        return 3;
    }

    if(base1 == base2) {
        return 0;
    }

    if((base1 == 0) && (base2 == 3)) {
        return 1;
    }

    if((base2 == 0) && (base1 == 3)) {
        return 1;
    }

    if((base1 == 1) && (base2 == 2)) {
        return 1;
    }

    if((base2 == 2) && (base1 == 1)) {
        return 1;
    }

    return 2;
}

int main(int argc, char *argv[] ) {

    int i, j;
    int *data1, *data2;
    int *result;
    int *local_data1, *local_data2, *local_result;
    int rows, rank, size;
    struct timeval  tv1, tv2;
    struct timeval mytv0, mytv3;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    rows = ceil((float)M / size); // Número de filas que procesa cada proceso

    if (rank == 0) {
        data1 = (int *) malloc(M*N*sizeof(int));
        data2 = (int *) malloc(M*N*sizeof(int));
        result = (int *) malloc(M*N*sizeof(int));
    }

    local_data1 = (int *) malloc(rows*N*sizeof(int)); // Reserva de memoria para las filas que procesa cada proceso
    local_data2 = (int *) malloc(rows*N*sizeof(int)); // Reserva de memoria para las filas que procesa cada proceso
    local_result = (int *) malloc(rows*sizeof(int)); // Reserva de memoria para las filas que procesa cada proceso

    if (rank == 0) {
        /* Initialize Matrices */
        for(i=0;i<M;i++) {
            for(j=0;j<N;j++) {
                /* random with 20% gap proportion */
                data1[i*N+j] = fast_rand();
                data2[i*N+j] = fast_rand();
            }
        }
    }

    gettimeofday(&mytv0, NULL);
    MPI_Scatter(data1, rows*N, MPI_INT, local_data1, rows*N, MPI_INT, 0, MPI_COMM_WORLD); // Distribución de las filas de la matriz entre los procesos
    MPI_Scatter(data2, rows*N, MPI_INT, local_data2, rows*N, MPI_INT, 0, MPI_COMM_WORLD); // Distribución de las filas de la matriz entre los procesos
    
    gettimeofday(&tv1, NULL);
    int auxRows;
    if (rank != size-1) {
        auxRows = rows;
    } else {
        auxRows = M - rows*(size - 1);
    }
    for(i=0;i<auxRows;i++) {
        local_result[i]=0;
        for(j=0;j<N;j++) {
            local_result[i] += base_distance(local_data1[i*N+j], local_data2[i*N+j]);
        }
    }
    gettimeofday(&tv2, NULL);

    MPI_Gather(local_result, rows, MPI_INT, result, rows, MPI_INT, 0, MPI_COMM_WORLD); // Recolección de los resultados de cada proceso
    gettimeofday(&mytv3, NULL);
    
    int microseconds = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);
    int myMicroseconds = ((mytv3.tv_usec - mytv0.tv_usec)+ 1000000 * (mytv3.tv_sec - mytv0.tv_sec)) - microseconds;

    /* Display result */
    if (DEBUG == 1) {
        if (rank == 0) {
            int checksum = 0;
            for(i=0;i<M;i++) {
                checksum += result[i];
            }
            printf("Checksum: %d\n ", checksum);
        }
    } else if (DEBUG == 2 && rank ==0) {
        for(i=0;i<M;i++) {
            printf(" %d \t ",result[i]);
        }
    } else {
        printf ("Computation time (seconds) = %lf\n", (double) microseconds/1E6);
        printf ("Colective's time (seconds) = %lf\n", (double) myMicroseconds/1E6);
    }

    if (rank == 0) {
        free(data1); free(data2); free(result);
    }

    free(local_data1); free(local_data2); free(local_result);
    MPI_Finalize();
    return 0;
}