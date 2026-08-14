#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double timeNow(){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  

  return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}

void linhaMajor(double **A, double *x, double *y, int n){
  for (int i=0; i < n ; i++){
    double index = 0.0;
    for (int j=0; j<n; j++){
      index += A[i][j] * x[j];
    }
    y[i] = index;
  }
}


void colunaMajor(double **A, double *x, double *y, int n){
  for (int i=0; i < n; i++){
    y[i] = 0.0;
  } 
  for (int j=0; j < n; j++){
    for (int i=0; i<n; i++){
      y[i] += A[i][j] * x[j];
    }
  }
}


double** carregarMatrix(int n){
  double **A = malloc(n * sizeof(double *));
  for (int i=0; i<n; i++){
    A[i] = malloc(n * sizeof(double));
    for (int j=0; j<n; j++){
      A[i][j] = (double)(i + j) * 0.5;
    }
  } 
  return A;
}


void limparMatriz(double **A, int n){
  for (int i=0; i<n; i++) free(A[i]);
  free(A);
}



int main(){
  int tamanho[] = {10, 100, 250, 500, 1000, 10000, 15000, 25000};

  int testes = sizeof(tamanho)/sizeof(tamanho[0]);

  printf("%-8s %-16s %-16s %-12s\n",
  "N", "Row Major (s)", "Col-Major (s)", "Razao (col/row)");
  printf("------------------------------------------------------\n");

  for (int k = 0; k < testes; k++){
    int n = tamanho[k];

    double **A = carregarMatrix(n);
    double *x = malloc(n * sizeof(double));
    double *y = malloc(n * sizeof(double));

    for (int i=0; i<n; i++) x[i] = 1.0;
    
    double comeco = timeNow(); 
    linhaMajor(A,x,y,n);
    double tempo_linha = timeNow() - comeco;

    comeco = timeNow();
    colunaMajor(A,x,y,n);
    double tempo_coluna = timeNow() - comeco;
    printf("%-8d %-16.6f %-16.6f %-12.2f\n", n, tempo_linha, tempo_coluna,tempo_coluna/tempo_linha);
 
      limparMatriz(A, n);
      free(x);
      free(y);
 
    }

  return 0;
}
