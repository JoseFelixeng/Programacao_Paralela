#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

#define TAM 1000000
#define ITER 200


int main(void){
  double *X = malloc(TAM * sizeof(double));
  double *Y = malloc(TAM * sizeof(double));
  double *Z = malloc(TAM * sizeof(double));

  
  for(int i = 0; i < TAM; i++){
    X[i] = (double)i;

  }

  for(int j =0; j < TAM; j++){
     Y[j] = (double)j;
  }

  double inicio= omp_get_wtime();

  #pragma omp parallel for
  for(int i = 0; i < TAM; i++){
    double acumulador = 0.0;
    for(int j = 0; j < ITER; j++){
      acumulador += sin(X[i]) * cos(Y[j]);
    }
    Z[i] = acumulador;
  }
  
  double fim = omp_get_wtime();

  printf("Tempo progcpu.c: %f (s) | threads=%d\n", fim - inicio, omp_get_max_threads());

  free(X);
  free(Y);
  free(Z);

  return 0; 
}
