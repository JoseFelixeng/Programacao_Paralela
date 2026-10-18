#include <stdio.h>
#include <string.h> 
#include "mpi.h"

int main(int argc, char *argv[]){
  int meu_ranque, num_procs;
  int origem, destino = 0, etiq = 1;

  char mensagem[200];

  MPI_Status estado;
  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

  if(meu_ranque != destino){
    sprintf(mensagem, "O Processo %d esta vivo!", meu_ranque);
    MPI_Send(mensagem, strlen(mensagem)+1, MPI_CHAR, destino, etiq, MPI_COMM_WORLD);
  }else{
    for (origem = 1; origem < num_procs; origem++){
      MPI_Recv(mensagem, 200, MPI_CHAR, origem, etiq, MPI_COMM_WORLD, &estado);
      printf("%s\n", mensagem);
    }
  }
  MPI_Finalize();
  printf("\n | O Programa foi Finalizado com Sucesso! |");

  return 0;

}
