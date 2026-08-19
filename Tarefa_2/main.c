#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double *alocarMatriz(int n){
  double *m = malloc((size_t)n * n * sizeof(double));
  for(int i = 0; i < n*n; i++){
    m[i] = (double)(rand() % 100) / 7.0;
  }
  return m;
}

double *alocarVetor(int n){
  double *v = malloc((size_t)n * sizeof(double));
  for(int i = 0; i < n; i++){
    v[i] = (double)(rand() % 100) / 7.0;
  }
  return v;
}

void majorRow(double *A, double *x, double *y, int n){
  for(int i = 0; i < n; i++){
      double soma = 0.0;
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
      y[i] += A[i* n + j] * x[j];
    }
  }
}

int main(){
  int n_min = 32;
  int n_max = 2048;
  int passo = 32;

    FILE *arquivo = fopen("resultados.csv", "w");

    if (arquivo == NULL) {
        printf("Erro ao criar o arquivo CSV.\n");
        return 1;
    }
  fprintf(arquivo, "N,Linhas (s),Colunas (s),Razao (Col/Lin)\n");
  printf("%-10s %-18s %-18s %-12s\n", "N", "Linhas (s)", "Colunas (s)", "Razao (Col/Lin)");
  printf("--------------------------------------------------------------\n");

  for(int k = n_min; k <= n_max; k += passo){
    double *A = alocarMatriz(k);
    double *x = alocarVetor(k);
    double *yl = malloc(k * sizeof(double));
    double *yc = malloc(k * sizeof(double));

    int reps;
    if (k <= 512) {
        reps = 20;
    } else if (k <= 1024) {
        reps = 5;
    } else {
        reps = 2;
    }
    
    double tLinhas = 1e18;
    double tColunas = 1e18;

    for (int m = 0; m < reps; m++){
      double testRow = medirTempo(majorRow, A, x, yl, k);
      if(testRow < tLinhas){
        tLinhas = testRow;
      }
    }

    for (int m = 0; m < reps; m++){
      double testColumn = medirTempo(majorColumn, A, x, yc, k);
      if(testColumn < tColunas){
        tColunas = testColumn;
      }
    }
    
    double razao = tColunas / tLinhas;
    // Mostra no terminal
    printf("%-10d %-18.6f %-18.6f %-12.2f\n", k, tLinhas, tColunas, razao);

    // Salva no CSV
    fprintf(arquivo, "%d,%.6f,%.6f,%.2f\n", k, tLinhas, tColunas, razao);

    free(A); 
    free(x); 
    free(yl); 
    free(yc);
  }

  // Fechar o arquivo
  fclose(arquivo);
  printf("\nResultados salvos em resultados.csv\n");

  return 0;
}
