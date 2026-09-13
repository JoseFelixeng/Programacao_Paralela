#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>     /* int32_t */
#include <omp.h>
#include <math.h>
#include <sys/stat.h>   /* mkdir */

/* ---------------------------------------------------------------------------
 * Simulacao de Navier-Stokes considerando APENAS viscosidade, em 3D:
 *
 *     du/dt = ALFA * (d2u/dx2 + d2u/dy2 + d2u/dz2)
 *
 * Diferencas finitas centradas (7 pontos) no espaco + Euler explicito no
 * tempo, no mesmo formato do template com omp.h/omp_get_wtime (arrays
 * globais u/u_next, funcao separada de copia) -- so que com as 3 fases que
 * o enunciado pede:
 *   1) fluido parado        -> verifica que o campo continua estavel (u=0)
 *   2) velocidade constante -> verifica que o campo continua estavel
 *   3) perturbacao local    -> observa se ela se difunde suavemente
 *
 * ALFA = 0.1 satisfaz o criterio de estabilidade explicito em 3D
 * (ALFA <= 1/6 ~ 0.167), entao nao ha necessidade de recalcular dt.
 *
 * NOVO: alem de imprimir max|u| no terminal, o programa agora GRAVA o
 * campo 3D em disco a cada INTERVALO_SNAPSHOT passos, em formato binario
 * compacto (float32, com subamostragem STRIDE_SNAPSHOT para manter os
 * arquivos pequenos). E esse arquivo que o visualizador em Python
 * (visualizador_cilindro.py --modo dados_c) le e projeta em 3D -- ou
 * seja, o Python passa a mostrar os DADOS REAIS gerados por este C, em
 * vez de reimplementar a simulacao.
 *
 * Formato do arquivo binario (little-endian, o padrao em x86/ARM):
 *   int32 nx_s, int32 ny_s, int32 nz_s   -> dimensoes da malha JA subamostrada
 *   float32 * (nx_s*ny_s*nz_s)           -> valores de u, na mesma ordem dos
 *                                           loops (i externo, j meio, k interno)
 *
 * Compilar:  gcc -O2 -Wall -fopenmp -o sim sim.c -lm
 * Rodar:     ./sim
 * --------------------------------------------------------------------------*/

#define N 200
#define PASSOS 100
#define ALFA 0.1
#define LOG_A_CADA 20

/* --- controle dos snapshots gravados em disco para o visualizador Python --- */
#define SALVAR_SNAPSHOTS   1        /* 1 = grava, 0 = desliga (so imprime no terminal) */
#define INTERVALO_SNAPSHOT 20       /* a cada quantos passos grava um arquivo */
#define STRIDE_SNAPSHOT    2        /* subamostragem: pega 1 a cada N pontos por eixo */
#define PASTA_SAIDA        "saida"

double u[N][N][N];
double u_next[N][N][N];

/* Fluido parado: u = 0 em todo o dominio */
void inicializar_parado(void) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                u[i][j][k] = 0.0;
}

/* Fluido com velocidade constante u0 */
void inicializar_constante(double u0) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                u[i][j][k] = u0;
}

/* Perturbacao: bloco central 20x20x20 recebe o valor informado
 * (mesma ideia do inicializar_fluido() do template) */
void adicionar_perturbacao(double valor) {
    for (int i = N/2 - 10; i < N/2 + 10; i++)
        for (int j = N/2 - 10; j < N/2 + 10; j++)
            for (int k = N/2 - 10; k < N/2 + 10; k++)
                u[i][j][k] = valor;
}

/* Copia u_next para u (mesma funcao atualizar_matriz_seq do template) */
void atualizar_matriz_seq(void) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                u[i][j][k] = u_next[i][j][k];
}

double valor_maximo(void) {
    double max_u = 0.0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                if (fabs(u[i][j][k]) > max_u) max_u = fabs(u[i][j][k]);
    return max_u;
}

/* Um passo de difusao viscosa 3D (mesma formula do template, so que com
 * o campo ja separado por fase em vez de rodar so uma vez) */
void passo_difusao_seq(void) {
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
    atualizar_matriz_seq();
}

/* Numero de pontos que sobram em cada eixo depois da subamostragem
 * (mesma conta que range(0, N, stride) faria em Python) */
static int tamanho_subamostrado(int n, int stride) {
    return (n + stride - 1) / stride;
}

/* Grava o campo u inteiro (subamostrado) em um arquivo binario, para o
 * visualizador em Python ler e projetar em 3D. */
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

    for (int i = 0; i < N; i += STRIDE_SNAPSHOT) {
        for (int j = 0; j < N; j += STRIDE_SNAPSHOT) {
            for (int k = 0; k < N; k += STRIDE_SNAPSHOT) {
                float valor = (float) u[i][j][k];
                fwrite(&valor, sizeof(float), 1, f);
            }
        }
    }

    fclose(f);
}

/* Roda PASSOS passos de difusao para a fase atual, imprimindo o progresso,
 * gravando snapshots binarios para o Python, e medindo o tempo com
 * omp_get_wtime (mesmo cronometro do template, pronto para comparar com a
 * versao paralela quando ela for escrita). "nome" tambem vira o prefixo dos
 * arquivos de snapshot (ex.: saida/perturbacao_0020.bin). */
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

        if (t < PASSOS) passo_difusao_seq();
    }
    double fim = omp_get_wtime();
    printf("[V1] tempo fase %s: %.4f s\n\n", nome, fim - inicio);
}

int main(void) {
    if (SALVAR_SNAPSHOTS) {
        mkdir(PASTA_SAIDA, 0777); /* ja existir nao e' erro para o programa */
        printf("Snapshots binarios serao gravados em '%s/' a cada %d passos\n"
               "(malha subamostrada a cada %d pontos por eixo -> %dx%dx%d por arquivo)\n\n",
               PASTA_SAIDA, INTERVALO_SNAPSHOT, STRIDE_SNAPSHOT,
               tamanho_subamostrado(N, STRIDE_SNAPSHOT),
               tamanho_subamostrado(N, STRIDE_SNAPSHOT),
               tamanho_subamostrado(N, STRIDE_SNAPSHOT));
    }

    printf("Malha %dx%dx%d | ALFA=%.3f | passos por fase=%d\n\n", N, N, N, ALFA, PASSOS);

    /* ---- 1) fluido parado ---- */
    printf("-- Fluido parado --\n");
    inicializar_parado();
    rodar_fase("parado");

    /* ---- 2) velocidade constante ---- */
    printf("-- Velocidade constante (u0 = 1.0) --\n");
    inicializar_constante(1.0);
    rodar_fase("constante");
    printf("(cai perto da borda por causa do contorno fixo u=0 -- esperado)\n\n");

    /* ---- 3) perturbacao ---- */
    printf("-- Perturbacao (bloco central 20x20x20 = 100.0) --\n");
    inicializar_parado();
    adicionar_perturbacao(100.0);
    rodar_fase("perturbacao");
    printf("-> max|u| cai suavemente, sem oscilacoes: a perturbacao esta se difundindo.\n");

    return 0;
}