//Anton Lopez Nunez y Carlos Martinez Rabunal
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

void inicializaCadena(char *cadena, int n){
  int i;
  for(i=0; i<n/2; i++){ // La mitad de la cadena son A's
    cadena[i] = 'A';
  }
  for(i=n/2; i<3*n/4; i++){ // El siguiente 25% de la cadena son C's
    cadena[i] = 'C';
  }
  for(i=3*n/4; i<9*n/10; i++){ // El siguiente 15% de la cadena son G's
    cadena[i] = 'G';
  }
  for(i=9*n/10; i<n; i++){ // El ultimo 10% de la cadena son T's
    cadena[i] = 'T';
  }
}

int main(int argc, char *argv[]) {

    if(argc != 3) {
        printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
        exit(1); 
    }
    
    int i, n, count=0;
    char *cadena;
    char L;
    int rank, numprocs; // Identificador del proceso y numero de procesos
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    if (rank == 0) {
        n = atoi(argv[1]); // Tamaño de la cadena
        L = *argv[2]; // Letra que se quiere contar
    }
    
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD); // Enviamos el tamaño de la cadena a todos los procesos
    MPI_Bcast(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD); // Enviamos la letra que se quiere contar a todos los procesos
    
    cadena = (char *) malloc(n*sizeof(char));
    inicializaCadena(cadena, n);
    

    for(i=rank; i<n; i += numprocs) { // Contamos las apariciones de la letra L en la cadena
        if(cadena[i] == L){
            count++;
        }
    }

    int aux = 0;
    MPI_Reduce(&count, &aux, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); // Sumamos todos los count de los procesos

    if(rank == 0) {
        printf("El numero de apariciones en el proceso %d de la letra %c es %d\n", rank, L, count); // Imprimimos el resultado del proceso 0 antes de la suma de todos los procesos
        printf("El numero de apariciones de la letra %c es %d\n", L, aux); // Imprimimos el resultado final (la suma de todos)
    } else {
        printf("El numero de apariciones en el proceso %d de la letra %c es %d\n", rank, L, count); // Imprimimos el resultado de los procesos distintos al 0
    }
    
    free(cadena);
    MPI_Finalize();
    exit(0);
}