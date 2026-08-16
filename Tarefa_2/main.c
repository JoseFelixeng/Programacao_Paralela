#include <stdio.h>
#include <stdlib.h>
#include <time.h>
<<<<<<< HEAD
=======

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
>>>>>>> 5bda72401cca2e23149f00f8a87d7af11c960cd3

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
<<<<<<< HEAD
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
=======
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
>>>>>>> 5bda72401cca2e23149f00f8a87d7af11c960cd3

  return 0;
}
