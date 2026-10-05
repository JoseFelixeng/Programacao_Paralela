// Código Heat.c base vindo direto do diretorio usando OpenMP com GPU 
// Dois dados na entrada Numero de Celulas e Passo 
// .heat.c 100 10

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>

// Constantes usadas no programa
#define PI acos(-1.0) // Pi
#define LINE "--------------------\n" // Linha para saida formatada

// Definição das funções
void initial_value(const int n, const double dx, const double length, double * restrict u);
void zero(const int n, double * restrict u);
void solve(const int n, const double alpha, const double dx, const double dt, const double * restrict u, double * restrict u_tmp);
double solution(const double t, const double x, const double y, const double alpha, const double length);
double l2norm(const int n, const double * restrict u, const int nsteps, const double dt, const double alpha, const double dx, const double length);

// Main
int main(int argc, char *argv[]) {

  // Inicia o tempo de execução do 
  double start = omp_get_wtime();
  int n = 1000; // tamanho do problema, forma um grid nxn 
  int nsteps = 10; //numero de passos no tempo


  // Checa o numero do argumento, printa e sai caso não seja o correto
  if (argc == 3) {
    // Configura o tamanho do primeiro argumento 
    n = atoi(argv[1]);
    if (n < 0) {
      fprintf(stderr, "Error: n deve ser positivo\n");
      exit(EXIT_FAILURE);
    }
    //configura o numero de passos de tempo 
    nsteps = atoi(argv[2]);
    if (nsteps < 0) {
      fprintf(stderr, "Error: nstep deve ser positivo\n");
      exit(EXIT_FAILURE);
    }
  }

  // Configura a definição do problema
  double alpha = 0.1;          // coeficiente da equação de difusão de calor
  double length = 1000.0;      // tamanho físico do domínio: quadrado de lado x lado
  double dx = length / (n+1);  // tamanho físico de cada célula (+1, pois não simulamos as fronteiras, já que elas são fornecidas)
  double dt = 0.5 / nsteps;    // ntervalo de tempo (tempo total de 0,5 s)


  // A estabilidade requer que dt/(dx^2) <= 0.5,
  double r = alpha * dt / (dx * dx);

  // Detalhes da execução
  printf("\n");
  printf(" MMS heat equation\n\n");
  printf(LINE);
  printf("Problem input\n\n");
  printf(" Grid size: %d x %d\n", n, n);
  printf(" Cell width: %E\n", dx);
  printf(" Grid length: %lf x %lf\n", length, length);
  printf("\n");
  printf(" Alpha: %E\n", alpha);
  printf("\n");
  printf(" Steps: %d\n", nsteps);
  printf(" Total time: %E\n", dt*(double)nsteps);
  printf(" Time step: %E\n", dt);
  printf(LINE);

  // Checando a estabilidade 
  printf("Stability\n\n");
  printf(" r value: %lf\n", r);
  if (r > 0.5)
    printf(" Warning: unstable\n");
  printf(LINE);


  // Alocando o grid nxn 
  double *u     = malloc(sizeof(double)*n*n);
  double *u_tmp = malloc(sizeof(double)*n*n);
  double *tmp;

  // Configurando a inicialização 
  initial_value(n, dx, length, u);
  zero(n, u_tmp);

  // Execute a simulação ao longo dos passos de tempo utilizando o esquema explícito
  double tic = omp_get_wtime(); //Inicie o temporizador da rotina `solve`
  
  for (int t = 0; t < nsteps; ++t) {
    // Chame o *kernel* de resolução (*solve kernel*), Calcula u_tmp no próximo passo de tempo,com base no valor de u no passo de tempo atual
    solve(n, alpha, dx, dt, u, u_tmp);
    // Ponteiro de troca
    tmp = u;
    u = u_tmp;
    u_tmp = tmp;
  }

  double toc = omp_get_wtime(); // fim da contagem de tempo
  // Verifique a norma L2 da solução calculada em relação à solução *conhecida* do esquema MMS
  double norm = l2norm(n, u, nsteps, dt, alpha, dx, length); 
  double stop = omp_get_wtime(); // Para o tempo total

  //Resultados
  printf("Results\n\n");
  printf("Error (L2norm): %E\n", norm);
  printf("Solve time (s): %lf\n", toc-tic);
  printf("Total time (s): %lf\n", stop-start);
  printf(LINE);

  // libera a memoria
  free(u);
  free(u_tmp);
}


// Define a malha com um valor inicial, determinado pelo esquema MMS
void initial_value(const int n, const double dx, const double length, double * restrict u) {
  double y = dx;
  for (int j = 0; j < n; ++j) {
    double x = dx; // Posição física x
    for (int i = 0; i < n; ++i) {
      u[i+j*n] = sin(PI * x / length) * sin(PI * y / length);
      x += dx;
    }
    y += dx; // Posição física y
  }

}


// Zere o array u
void zero(const int n, double * restrict u) {
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      u[i+j*n] = 0.0;
    }
  }

}


// Compute the next timestep, given the current timestep
void solve(const int n, const double alpha, const double dx, const double dt, const double * restrict u, double * restrict u_tmp) {
  // Multiplicador constante de diferenças finitas
  
  const double r = alpha * dt / (dx * dx);
  const double r2 = 1.0 - 4.0*r;
  
  // Itere sobre a grade *n* x *n*
  #pragma omp target map(tofrom: u[0:n*n], u_tmp[0: n*n])
  #pragma omp loop collapse(2)
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
      //Atualiza o stencil de 5 pontos, utilizando condições de contorno nas bordas do domínio. Os valores de contorno são zero porque a solução MMS é zero nesses pontos.
        u_tmp[i+j*n] =  r2 * u[i+j*n] +
        r * ((i < n-1) ? u[i+1+j*n] : 0.0) +
        r * ((i > 0)   ? u[i-1+j*n] : 0.0) +
        r * ((j < n-1) ? u[i+(j+1)*n] : 0.0) +
        r * ((j > 0)   ? u[i+(j-1)*n] : 0.0);
      }
    }

  
}

// Solução correta fornecida pela solução manufaturada.
double solution(const double t, const double x, const double y, const double alpha, const double length) {
  return exp(-2.0*alpha*PI*PI*t/(length*length)) * sin(PI*x/length) * sin(PI*y/length);
}


// Calcula a norma L2 da grade calculada em relação à solução conhecida via MMS. A solução conhecida é a mesma que a função de contorno.
double l2norm(const int n, const double * restrict u, const int nsteps, const double dt, const double alpha, const double dx, const double length) {
  double time = dt * (double)nsteps;// Final (real) tempo da simulação0
  double l2norm = 0.0; // Erro da norma L2
  // Percorra a grade e calcule a diferença entre as soluções calculadas e as conhecidas como uma norma L2.
  double y = dx;
  for (int j = 0; j < n; ++j) {
    double x = dx;
    for (int i = 0; i < n; ++i) {
      double answer = solution(time, x, y, alpha, length);
      l2norm += (u[i+j*n] - answer) * (u[i+j*n] - answer);
      x += dx;
    }
    y += dx;
  }
  return sqrt(l2norm);
}
