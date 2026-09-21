/*
 * navier_stokes_seq_teste.c
 * -----------------------------------------------------------------------
 * Versao SEQUENCIAL (sem OpenMP) de COMPROVACAO da difusao viscosa 3D.
 * Mesmo solver do codigo sequencial do relatorio, com tres parametros de
 * compilacao para testar a estabilidade do campo:
 *
 *      du/dt = nu * (d2u/dx2 + d2u/dy2 + d2u/dz2)     (FTCS, 7 pontos)
 *
 *   U0        velocidade inicial uniforme (padrao 0.0 = fluido parado)
 *   AMPLITUDE amplitude da perturbacao gaussiana (padrao 2.0; 0.0 = sem perturbacao)
 *   FATOR_DT  dt = FATOR_DT * limite de estabilidade (padrao 0.4; > 1.0 e instavel)
 *
 * Paredes com u = 0 (Dirichlet). Ao final o programa confere o principio do
 * maximo: o campo deve ficar dentro de [min, max] do campo inicial e finito.
 *
 * Compilacao (um caso por compilacao):
 *   gcc -O2 -o seq_teste navier_stokes_seq_teste.c -lm
 *   gcc -O2 -DAMPLITUDE=0.0 -DU0=0.0 -o seq_parado    navier_stokes_seq_teste.c -lm
 *   gcc -O2 -DAMPLITUDE=0.0 -DU0=1.0 -o seq_constante navier_stokes_seq_teste.c -lm
 *   gcc -O2 -DFATOR_DT=1.2           -o seq_instavel  navier_stokes_seq_teste.c -lm
 *
 * Execucao:
 *   ./seq_teste [NX] [NY] [NZ] [N_STEPS] [saida.csv] [MAX_PTS] [N_SNAPS]
 *   (use /dev/null como saida.csv para nao gerar arquivo)
 *
 * CSV: passo,t,i,j,k,x,y,z,u
 * -----------------------------------------------------------------------
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ------------------------- Parametros fisicos e padroes ------------------------- */
#define LX 1.0
#define LY 1.0
#define LZ 1.0
#define NU 0.01

#ifndef AMPLITUDE
#define AMPLITUDE 2.0
#endif
#ifndef U0
#define U0 0.0
#endif
#ifndef FATOR_DT
#define FATOR_DT 0.4
#endif
#define LARGURA   0.03

#define NX_PADRAO 64
#define NY_PADRAO 64
#define NZ_PADRAO 64
#define N_STEPS_PADRAO 200
#define CSV_PADRAO "difusao3d_teste_seq.csv"
#define MAX_PTS_PADRAO 48
#define N_SNAPS_PADRAO 5

static double agora_em_segundos(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

static inline long idx(int i, int j, int k, int NY, int NZ) {
    return ((long) i * NY + j) * NZ + k;
}

static double *aloca_campo(int NX, int NY, int NZ) {
    double *campo = (double *) calloc((size_t) NX * NY * NZ, sizeof(double));
    if (campo == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memoria.\n");
        exit(EXIT_FAILURE);
    }
    return campo;
}

/* Contornos em zero (Dirichlet). O passo nunca escreve nas faces, entao
 * basta aplicar uma vez no campo inicial (o outro buffer vem zerado). */
static void aplica_contorno(double *c, int NX, int NY, int NZ) {
    for (int i = 0; i < NX; i++)
        for (int j = 0; j < NY; j++) {
            c[idx(i, j, 0, NY, NZ)] = 0.0;
            c[idx(i, j, NZ - 1, NY, NZ)] = 0.0;
        }
    for (int i = 0; i < NX; i++)
        for (int k = 0; k < NZ; k++) {
            c[idx(i, 0, k, NY, NZ)] = 0.0;
            c[idx(i, NY - 1, k, NY, NZ)] = 0.0;
        }
    for (int j = 0; j < NY; j++)
        for (int k = 0; k < NZ; k++) {
            c[idx(0, j, k, NY, NZ)] = 0.0;
            c[idx(NX - 1, j, k, NY, NZ)] = 0.0;
        }
}

/* Um passo de difusao explicita (FTCS) -- SEQUENCIAL. */
static void passo_difusao(const double *c, double *cn, double dt,
                          double dx, double dy, double dz, int NX, int NY, int NZ) {
    const double kx = NU * dt / (dx * dx);
    const double ky = NU * dt / (dy * dy);
    const double kz = NU * dt / (dz * dz);

    for (int i = 1; i < NX - 1; i++) {
        for (int j = 1; j < NY - 1; j++) {
            for (int k = 1; k < NZ - 1; k++) {
                long p = idx(i, j, k, NY, NZ);
                double centro = c[p];
                cn[p] = centro
                      + kx * (c[idx(i + 1, j, k, NY, NZ)] - 2.0 * centro + c[idx(i - 1, j, k, NY, NZ)])
                      + ky * (c[idx(i, j + 1, k, NY, NZ)] - 2.0 * centro + c[idx(i, j - 1, k, NY, NZ)])
                      + kz * (c[idx(i, j, k + 1, NY, NZ)] - 2.0 * centro + c[idx(i, j, k - 1, NY, NZ)]);
            }
        }
    }
}

/* Gaussiana 3D no centro do dominio (somada ao campo ja existente). */
static void aplica_perturbacao_gaussiana(double *u, double dx, double dy, double dz,
                                          int NX, int NY, int NZ) {
    for (int i = 0; i < NX; i++)
        for (int j = 0; j < NY; j++)
            for (int k = 0; k < NZ; k++) {
                double x = i * dx - 0.5 * LX;
                double y = j * dy - 0.5 * LY;
                double z = k * dz - 0.5 * LZ;
                double r2 = x * x + y * y + z * z;
                u[idx(i, j, k, NY, NZ)] += AMPLITUDE * exp(-r2 / (2.0 * LARGURA * LARGURA));
            }
    aplica_contorno(u, NX, NY, NZ);
}

/* Minimo, maximo e soma do campo. */
static void estatisticas(const double *u, long n, double *mn, double *mx, double *soma) {
    double a = u[0], b = u[0], s = 0.0;
    for (long p = 0; p < n; p++) {
        if (u[p] < a) a = u[p];
        if (u[p] > b) b = u[p];
        s += u[p];
    }
    *mn = a; *mx = b; *soma = s;
}

/* Grava um snapshot (subamostrado com passo ex,ey,ez) no CSV. */
static long grava_snapshot(FILE *f, const double *u, int NX, int NY, int NZ,
                           double dx, double dy, double dz, long passo, double t,
                           int ex, int ey, int ez) {
    long linhas = 0;
    for (int i = 0; i < NX; i += ex)
        for (int j = 0; j < NY; j += ey)
            for (int k = 0; k < NZ; k += ez) {
                fprintf(f, "%ld,%.6g,%d,%d,%d,%.5f,%.5f,%.5f,%.6g\n",
                        passo, t, i, j, k, i * dx, j * dy, k * dz,
                        u[idx(i, j, k, NY, NZ)]);
                linhas++;
            }
    return linhas;
}

int main(int argc, char **argv) {
    int NX = (argc > 1) ? atoi(argv[1]) : NX_PADRAO;
    int NY = (argc > 2) ? atoi(argv[2]) : NY_PADRAO;
    int NZ = (argc > 3) ? atoi(argv[3]) : NZ_PADRAO;
    long N_STEPS = (argc > 4) ? atol(argv[4]) : N_STEPS_PADRAO;
    const char *arq_csv = (argc > 5) ? argv[5] : CSV_PADRAO;
    int max_pts = (argc > 6) ? atoi(argv[6]) : MAX_PTS_PADRAO;
    int n_snaps = (argc > 7) ? atoi(argv[7]) : N_SNAPS_PADRAO;

    if (NX < 3 || NY < 3 || NZ < 3 || N_STEPS < 1 || max_pts < 2 || n_snaps < 2) {
        fprintf(stderr, "Uso: %s [NX>=3] [NY>=3] [NZ>=3] [N_STEPS>=1] [saida.csv] [MAX_PTS>=2] [N_SNAPS>=2]\n", argv[0]);
        return EXIT_FAILURE;
    }

    double dx = LX / (NX - 1);
    double dy = LY / (NY - 1);
    double dz = LZ / (NZ - 1);
    double limite_estabilidade = 0.5 / (NU * (1.0 / (dx * dx) + 1.0 / (dy * dy) + 1.0 / (dz * dz)));
    double dt = FATOR_DT * limite_estabilidade;

    double *u = aloca_campo(NX, NY, NZ);
    double *u_novo = aloca_campo(NX, NY, NZ);
    long total_pontos = (long) NX * NY * NZ;

    /* campo inicial: velocidade uniforme U0 + perturbacao (paredes em zero) */
    for (long p = 0; p < total_pontos; p++) u[p] = U0;
    aplica_perturbacao_gaussiana(u, dx, dy, dz, NX, NY, NZ);

    double min0, max0, soma0;
    estatisticas(u, total_pontos, &min0, &max0, &soma0);

    FILE *f = fopen(arq_csv, "w");
    if (f == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'.\n", arq_csv);
        return EXIT_FAILURE;
    }
    fprintf(f, "passo,t,i,j,k,x,y,z,u\n");

    int ex = (NX + max_pts - 1) / max_pts;
    int ey = (NY + max_pts - 1) / max_pts;
    int ez = (NZ + max_pts - 1) / max_pts;

    fprintf(stderr, "Malha = %d x %d x %d (%.2f milhoes de pontos) | passos = %ld | SEQUENCIAL | dt = %.3e\n",
            NX, NY, NZ, total_pontos / 1e6, N_STEPS, dt);
    fprintf(stderr, "Teste: U0 = %g | AMPLITUDE = %g | FATOR_DT = %g\n",
            (double) U0, (double) AMPLITUDE, (double) FATOR_DT);
    fprintf(stderr, "soma inicial = %.10f\n", soma0);

    double tempo_calculo = 0.0;   /* so os passos; a gravacao do CSV fica de fora */
    long linhas = 0;
    int s = 0;

    for (long passo = 0; passo <= N_STEPS; passo++) {
        /* snapshots igualmente espacados entre o passo 0 e o final */
        if (s < n_snaps && passo == (long) s * N_STEPS / (n_snaps - 1)) {
            linhas += grava_snapshot(f, u, NX, NY, NZ, dx, dy, dz, passo, passo * dt, ex, ey, ez);
            while (s < n_snaps && passo == (long) s * N_STEPS / (n_snaps - 1)) s++;
        }
        if (passo == N_STEPS) break;

        double inicio = agora_em_segundos();
        passo_difusao(u, u_novo, dt, dx, dy, dz, NX, NY, NZ);
        double *tmp = u; u = u_novo; u_novo = tmp;      /* troca de ponteiros, sem memcpy */
        tempo_calculo += agora_em_segundos() - inicio;
    }
    fclose(f);

    double minf, maxf, somaf;
    estatisticas(u, total_pontos, &minf, &maxf, &somaf);
    const double tol = 1e-12;
    int ok = isfinite(somaf) && minf >= min0 - tol && maxf <= max0 + tol;

    fprintf(stderr, "u_min = %.6e | u_max = %.6e\n", minf, maxf);
    fprintf(stderr, "(soma de u) = %.10f\n", somaf);
    fprintf(stderr, "Estabilidade (campo finito e dentro de [%.3g, %.3g]): %s\n", min0, max0, ok ? "OK" : "FALHOU");
    fprintf(stderr, "csv salvo como: %s: %d snapshots, %ld linhas (amostragem %dx%dx%d)\n", arq_csv, s, linhas, ex, ey, ez);
    printf("Tempo: %.6f s\n", tempo_calculo);

    free(u);
    free(u_novo);
    return ok ? 0 : 2;
}
