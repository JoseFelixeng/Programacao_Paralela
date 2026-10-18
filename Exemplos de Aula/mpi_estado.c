#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define MAX 100 

int main(int argc, char *argv[])
{
  int meu_ranque, total_num, etiq = 0;
  int origem = 0, destino=1, numeros[MAX];
  
  MPI_Status estado;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);

  if(meu_ranque == origem){
    srand(MPI_Wtime());
    total_num = (rand()/(float)RAND_MAX) * MAX;
    MPI_Send(numeros, total_num, MPI_INT, destino, etiq, MPI_COMM_WORLD);
    printf("Processo %d enviou %d numeros para o processo 1. o estado de origem é %d e sua etiqueta %d \n", origem, total_num, estado.MPI_SOURCE, estado.MPI_TAG);
  }
  
  if(meu_ranque == destino){
    MPI_Recv(numeros, MAX, MPI_INT, origem, etiq, MPI_COMM_WORLD, &estado);
    MPI_Get_count(&estado, MPI_INT, &total_num);
    printf("Processo %d recebeu %d números de processso 0. Origem da mensagem = %d, etiqueta = %d \n", destino, total_num, estado.MPI_SOURCE, estado.MPI_TAG);

  }

  MPI_Finalize();

  return 0; 
}
