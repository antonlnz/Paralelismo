// Anton Lopez Nunez y Carlos Martinez Rabunal
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <errno.h>

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

int myPow(int x, unsigned int y) {  // Funcion para elevar un int a un int(+).
    if (y == 0) // Caso base
        return 1;
    else if ((y % 2) == 0) // Si "y" es par
        return myPow(x, y / 2) * myPow(x, y / 2); // x^y = (x^2)^(y/2)
    else // Si "y" es impar
        return x * myPow(x, y / 2) * myPow(x, y / 2); // x^y = x * (x^2)^(y/2)
}

int MPI_BinomialBcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) { // Equivalente a MPI_Bcast implementada con arbol binomial
    int i = 0, rank, numprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    if (rank >= numprocs) { // Si el rank es mayor que el numero de procesos devuelve error
        printf("Invalid rank value\n");
        return MPI_ERR_RANK;
    }

    if (datatype != MPI_INT && datatype != MPI_CHAR) { // Si el datatype no es int devuelve error
        printf("Invalid datatype value\n");
        return MPI_ERR_TYPE;
    }

    if (root != 0) { // Si el root no es 0 devuelve error
        printf("Invalid root value\n");
        return MPI_ERR_ROOT;
    }

    if (rank != 0) {
        if (MPI_Recv(buffer, count, datatype, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, MPI_STATUS_IGNORE)) { // Recibe el mensaje del proceso anterior en el arbol binomial
            printf("MPI_Recv function returned error\n");
            return MPI_ERR_OP;
        }
    }

    while (1) { // Envía el mensaje a todos los procesos
        if (rank < myPow(2, i)) { // Si el proceso es menor que 2^i, envía el mensaje al proceso 2^i
            if(rank + myPow(2, i) >= numprocs) // Si el proceso 2^i no existe, no envía el mensaje
                break;
            if (MPI_Send(buffer, count, datatype, rank + myPow(2, i), 0, comm)) { // Envía el mensaje al proceso 2^i
                printf("MPI_Send function returned error\n");
                return MPI_ERR_OP;
            }
        }
        i++;
    }
    return MPI_SUCCESS;
}

int MPI_FlattreeReduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) { // Nuestra implementacion de MPI_Reduce
    int i, rank, numprocs;
    int rec_data;
    int *acc = recvbuf;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    if (rank >= numprocs) { // Si el rank es mayor que el numero de procesos devuelve error
        printf("Invalid rank value\n");
        return MPI_ERR_RANK;
    }

    if (datatype != MPI_INT && datatype != MPI_CHAR) { // Si el datatype no es int devuelve error
        printf("Invalid datatype value\n");
        return MPI_ERR_TYPE;
    }

    if (root != 0) {
        printf("Invalid root value\n");
        return MPI_ERR_ROOT;
    }

    if(rank == root) { // El proceso root recibe los datos de todos los procesos y los suma
        // Recibe el count de si mismo
        *acc = *(int *)sendbuf; // Inicializamos el acumulador con el valor del proceso root
        // Recibe el count de todos los demas procesos y los suma a su propio count
        for(i = 0; i < numprocs; i++) { // Recibe el count de todos los procesos
            if(i != root) { // Si el proceso no es root, recibe el count
                if(MPI_Recv(&rec_data, count, datatype, i, 0, comm, MPI_STATUS_IGNORE)) { // Recibe el count del proceso i
                    printf("MPI_Recv function returned error\n");
                    return MPI_ERR_OP;
                }
                *acc += rec_data; // Suma el count recibido al acumulador
            }
        }
    } else {
        if(MPI_Send(sendbuf, count, datatype, root, 0, comm)) { // Envía el count al proceso root
            printf("MPI_Send function returned error\n");
            return MPI_ERR_OP;
        }
    }
    return MPI_SUCCESS;
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
    
    MPI_BinomialBcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD); // Enviamos el tamaño de la cadena a todos los procesos
    MPI_BinomialBcast(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD); // Enviamos la letra que se quiere contar a todos los procesos
    
    cadena = (char *) malloc(n*sizeof(char));
    inicializaCadena(cadena, n);
    

    for(i=rank; i<n; i += numprocs) { // Contamos las apariciones de la letra L en la cadena
        if(cadena[i] == L){
            count++;
        }
    }

    int aux = 0;
    MPI_FlattreeReduce(&count, &aux, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); // Sumamos todos los count de los procesos

    if(rank == 0) {
        //printf("El numero de apariciones en el proceso %d de la letra %c es %d\n", rank, L, count); // Imprimimos el resultado del proceso 0 antes de la suma de todos los procesos
        printf("El numero de apariciones de la letra %c es %d\n", L, aux); // Imprimimos el resultado final (la suma de todos)
    } else {
        //printf("El numero de apariciones en el proceso %d de la letra %c es %d\n", rank, L, count); // Imprimimos el resultado de los procesos distintos al 0
    }
    
    free(cadena);
    MPI_Finalize();
    exit(0);
}