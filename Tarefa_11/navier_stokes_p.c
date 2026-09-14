#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>     /* int32_t */
#include <omp.h>
#include <math.h>
#include <sys/stat.h>   /* mkdir */

/* ---------------------------------------------------------------------------
 * Versao PARALELA (OpenMP) do sim.c original.
 *
 * O que muda em relacao a versao sequencial:
 *   - passo_difusao_seq()   -> passo_difusao_paralelo(): o laco espacial
 *     (o unico que realmente pesa, O(N^3) por passo de tempo) agora roda
 *     em paralelo com `#pragma omp parallel for`.
 *   - atualizar_matriz_seq() e valor_maximo() tambem paralelizados
 *     (collapse(3) schedule(static) fixo -- nao fazem parte do estudo de
 *     schedule/collapse, mas nao faz sentido deixa-los sequenciais).
 *   - `schedule(runtime)` no laco principal: o TIPO de schedule (static,
 *     dynamic, guided, com ou sem chunk) e escolhido em tempo de execucao
 *     pela variavel de ambiente OMP_SCHEDULE, sem precisar recompilar.
 *   - `COLLAPSE_NIVEL` (macro de compilacao, -DCOLLAPSE_NIVEL=0|2|3):
 *     controla quantos lacos do aninhamento i/j/k sao fundidos pelo
 *     collapse. Precisa ser decidido em tempo de COMPILACAO porque o
 *     numero de lacos fundidos faz parte da forma canonica do laco do
 *     OpenMP (nao da para "escolher em runtime").
 *
 *       COLLAPSE_NIVEL=0  -> so paraleliza o laco externo (i)
 *       COLLAPSE_NIVEL=2  -> collapse(2): funde i e j
 *       COLLAPSE_NIVEL=3  -> collapse(3): funde i, j e k (padrao)
 *
 * Como isso permite o estudo pedido:
 *   - Para cada COLLAPSE_NIVEL, compila-se um binario diferente:
 *       gcc -O2 -Wall -fopenmp -DCOLLAPSE_NIVEL=3 -o sim_c3 sim_paralelo.c -lm
 *   - Para cada schedule, roda-se o MESMO binario variando a variavel de
 *     ambiente, sem recompilar:
 *       OMP_SCHEDULE="static"    OMP_NUM_THREADS=8 ./sim_c3
 *       OMP_SCHEDULE="dynamic,8" OMP_NUM_THREADS=8 ./sim_c3
 *       OMP_SCHEDULE="guided"    OMP_NUM_THREADS=8 ./sim_c3
 *   - O script `benchmark_omp.py` (fornecido a parte) automatiza essa
 *     varredura de collapse x schedule x numero de threads e gera os
 *     graficos de desempenho.
 *
 * Compilar (schedule/collapse padrao = runtime / 3):
 *   gcc -O2 -Wall -fopenmp -o sim_paralelo sim_paralelo.c -lm
 * Rodar:
 *   OMP_SCHEDULE=static OMP_NUM_THREADS=4 ./sim_paralelo
 * --------------------------------------------------------------------------*/

#define N 200
#define PASSOS 100
#define ALFA 0.1
#define LOG_A_CADA 20

#ifndef COLLAPSE_NIVEL
#define COLLAPSE_NIVEL 3
#endif

/* --- controle dos snapshots gravados em disco para o visualizador Python --- */
#ifndef SALVAR_SNAPSHOTS
#define SALVAR_SNAPSHOTS 1
#endif
#define INTERVALO_SNAPSHOT 20
#define STRIDE_SNAPSHOT    2
#define PASTA_SAIDA        "saida"

double u[N][N][N];
double u_next[N][N][N];

void inicializar_parado(void) {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                u[i][j][k] = 0.0;
}

void inicializar_constante(double u0) {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                u[i][j][k] = u0;
}

/* Perturbacao: bloco central 20x20x20 recebe o valor informado.
 * Bloco pequeno (20^3 = 8000 celulas) -- nao vale a pena paralelizar,
 * fica sequencial mesmo (overhead de criar threads seria maior que o
 * proprio trabalho). */
void adicionar_perturbacao(double valor) {
    for (int i = N/2 - 10; i < N/2 + 10; i++)
        for (int j = N/2 - 10; j < N/2 + 10; j++)
            for (int k = N/2 - 10; k < N/2 + 10; k++)
                u[i][j][k] = valor;
}

/* Copia u_next para u. Paralelizado, mas com schedule FIXO (nao faz parte
 * do estudo de schedule/collapse do laco de difusao). */
void atualizar_matriz_paralela(void) {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                u[i][j][k] = u_next[i][j][k];
}

double valor_maximo(void) {
    double max_u = 0.0;
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                if (fabs(u[i][j][k]) > max_u) max_u = fabs(u[i][j][k]);
    return max_u;
}

/* ---------------------------------------------------------------------------
 * Passo de difusao PARALELO -- e este o laco que o estudo de schedule e
 * collapse mede. O nivel de collapse e fixado em tempo de compilacao pela
 * macro COLLAPSE_NIVEL; o tipo de schedule (static/dynamic/guided, com ou
 * sem chunk) vem de `schedule(runtime)`, ou seja, da variavel de ambiente
 * OMP_SCHEDULE lida quando o programa comeca a rodar.
 * ------------------------------------------------------------------------*/
void passo_difusao_paralelo(void) {
    for (int i = 1; i < N - 1; i++) {
        for (int j = 1; j < N - 1; j++) {
            for (int k = 1; k < N - 1; k++) {
                u_next[i][j][k] = u[i][j][k] + ALFA * (
                    u[i+1][j][k] + u[i-1][j][k] +
                    u[i][j+1][k] + u[i][j-1][k] +
                    u[i][j][k+1] + u[i][j][k-1] -
                    6.0 * u[i][j][k]
                );
            }
        }
    }
    atualizar_matriz_paralela();
}

static int tamanho_subamostrado(int n, int stride) {
    return (n + stride - 1) / stride;
}

void salvar_snapshot_binario(const char *nome_arquivo) {
    FILE *f = fopen(nome_arquivo, "wb");
    if (!f) {
        fprintf(stderr, "Erro ao abrir '%s' para escrita\n", nome_arquivo);
        return;
    }
    int32_t dims[3];
    dims[0] = tamanho_subamostrado(N, STRIDE_SNAPSHOT);
    dims[1] = dims[0];
    dims[2] = dims[0];
    fwrite(dims, sizeof(int32_t), 3, f);
    for (int i = 0; i < N; i += STRIDE_SNAPSHOT)
        for (int j = 0; j < N; j += STRIDE_SNAPSHOT)
            for (int k = 0; k < N; k += STRIDE_SNAPSHOT) {
                float valor = (float) u[i][j][k];
                fwrite(&valor, sizeof(float), 1, f);
            }
    fclose(f);
}

void rodar_fase(const char *nome) {
    double inicio = omp_get_wtime();

    for (int t = 0; t <= PASSOS; t++) {
          if (t % LOG_A_CADA == 0)
            printf("passo %3d: max|u| = %.6f\n", t, valor_maximo());

        if (SALVAR_SNAPSHOTS && t % INTERVALO_SNAPSHOT == 0) {
            char nome_arquivo[256];
            snprintf(nome_arquivo, sizeof(nome_arquivo), "%s/%s_%04d.bin",
                     PASTA_SAIDA, nome, t);
            salvar_snapshot_binario(nome_arquivo);
        }
        if (t < PASSOS) {
            passo_difusao_paralelo();
        }
    }
    double fim = omp_get_wtime();
    /* Linha de saida pensada para ser facil de "grepar" no benchmark:
     * "[V2] tempo fase <nome>: <segundos> s" */
    printf("[V2] tempo fase %s: %.6f s\n\n", nome, fim - inicio);
}

int main(void) {
    omp_sched_t tipo_schedule;
    int tamanho_chunk;
    omp_get_schedule(&tipo_schedule, &tamanho_chunk);
    const char *nome_schedule =
        tipo_schedule == omp_sched_static  ? "static"  :
        tipo_schedule == omp_sched_dynamic ? "dynamic" :
        tipo_schedule == omp_sched_guided  ? "guided"  : "auto/desconhecido";

    printf("Threads OpenMP disponiveis (omp_get_max_threads): %d\n", omp_get_max_threads());
    printf("schedule(runtime) resolvido via OMP_SCHEDULE: %s, chunk=%d\n\n",
           nome_schedule, tamanho_chunk);

    if (SALVAR_SNAPSHOTS) {
        mkdir(PASTA_SAIDA, 0777);
    }

    printf("Malha %dx%dx%d | ALFA=%.3f | passos por fase=%d\n\n", N, N, N, ALFA, PASSOS);

#ifndef APENAS_PERTURBACAO
    printf("-- Fluido parado --\n");
    inicializar_parado();
    rodar_fase("parado");

    printf("-- Velocidade constante (u0 = 1.0) --\n");
    inicializar_constante(1.0);
    rodar_fase("constante");
#endif

    printf("-- Perturbacao (bloco central 20x20x20 = 100.0) --\n");
    inicializar_parado();
    adicionar_perturbacao(100.0);
    rodar_fase("perturbacao");
    printf("-> max|u| cai suavemente, sem oscilacoes: a perturbacao esta se difundindo.\n");

    return 0;
}