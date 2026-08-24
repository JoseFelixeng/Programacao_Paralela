#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

#define TAM 100000000

int main(void){
  double *X = malloc(TAM * sizeof(double));
  double *Y = malloc(TAM * sizeof(double));
  double *Z = malloc(TAM * sizeof(double));

  
  for(int i = 0; i < TAM; i++){
    X[i] = 1.0;
    Y[i] = 2.0;
  }


  double inicio = omp_get_wtime();
  
  #pragma omp parallel for
  for(int i = 0; i < TAM; i++){
    Z[i] = X[i] + Y[i];
  }
  
  double fim = omp_get_wtime();

  double soma = 0.0;
  for(int i = 0; i < TAM; i++){
    soma += Z[i];
  }

  printf("Tempo progcpu.c: %f (s) | threads=%d  | Valor da soma(Z)=%e\n", fim - inicio, omp_get_max_threads(), soma);

  free(X);
  free(Y);
  free(Z);

  return 0; 
}


