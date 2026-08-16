#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double *alocarMatriz(int n){
  double *m = malloc((size_t)n * n * sizeof(double));
  for(int i = 0; i < n; i++){
    m[i] = (double)(rand() % 100) / 7.0;
  }
  return m;
}

double *alocarVetor(int n){
  double *v = malloc((size_t)n * n * sizeof(double));
  for(int i = 0; i < n; i++){
    v[i] = (double)(rand() % 100) / 7.0;
  }
  return v;
}

void majorRow(double *A, double *x, double *y, int n){
  double soma = 0.0;
  for(int i = 0; i < n; i++){
      for(int j = 0; j < n; j++){
        soma += A[i * n + j] * x[j];
      } 
      y[i] = soma;
  } 
}

double medirTempo(void (*func)(double*, double*, double*, int), double *A, double *x, double *y, int n){
  struct timespec inicio, fim; 
  clock_gettime(CLOCK_MONOTONIC, &inicio);
  func(A, x, y,n);
  clock_gettime(CLOCK_MONOTONIC, &fim);
  double tempo = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;

  return tempo;
}

void majorColumn(double *A, double *x, double *y, int n){
  for(int i =0; i < n; i++){
    y[i] = 0.0;
  }
  for(int j =0; j < n; j++){
    for(int i =0; i < n; i++){
      y[i] = A[i* n + j] * x[j];
    }
  }
}

int main(){
  int tamanho[] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
  int n_tamanho = sizeof(tamanho)/ sizeof(tamanho[0]);

  printf("%-10s %-18s %-18s %-12s\n", "N", "Linhas (s)", "Colunas (s)", "Razao (Col/Lin)");
  printf("--------------------------------------------------------------\n");

  for(int k = 0; k < n_tamanho; k++){
    int n = tamanho[k]; 
    double *A = alocarMatriz(n);
    double *x = alocarVetor(n);
    double *yl = malloc(n * sizeof(double));
    double *yc = malloc(n * sizeof(double));

    int reps;
    if (n <= 512) {
        reps = 20;
    } else if (n <= 1024) {
        reps = 5;
    } else {
        reps = 2;
    }
    double tLinhas = 1e18;
    double tColunas = 1e18;

    for (int m = 0; m < reps; m++){
      double testRow = medirTempo(majorRow, A, x, yl, n);
      if(testRow < tLinhas){
        tLinhas = testRow;
      }
    }

    for (int m = 0; m < reps; m++){
      double testColumn = medirTempo(majorColumn, A, x, yc, n);
      if(testColumn < tColunas){
        tColunas = testColumn;
      }
    }
    printf("%-10d %-18.6f %-18.6f %-12.2f\n", n, tLinhas, tColunas, tColunas/tLinhas);
    free(A); 
    free(x); 
    free(yl); 
    free(yc);
  }

  return 0;
}
