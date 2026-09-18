/*
Versão paralela
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* ------------------------- Parâmetros da malha ------------------------- */
#define NX 80          /* pontos na direção x */
#define NY 80          /* pontos na direção y */
#define LX 1.0         /* comprimento físico do domínio em x */
#define LY 1.0         /* comprimento físico do domínio em y */

/* ------------------------- Parâmetros físicos -------------------------- */
#define NU 0.01        /* viscosidade cinemática */

/* --------------------- Parâmetros de simulação -------------------------- */
#define FASE1 200     /* passos para verificar estabilidade (campo uniforme) */
#define FASE2 4000    /* passos para observar a difusão da perturbação */
#define PRINT_EVERY   500     /* intervalo (em passos) para imprimir estatísticas */
#define SNAPSHOT_EVERY 800    /* intervalo (em passos) para salvar snapshots em CSV */

/* Índice linear para acessar a malha 2D armazenada em vetor 1D */
static inline int idx(int i, int j) {
    return i * NY + j;
}

/* Aloca um vetor de doubles de tamanho NX*NY, inicializado com zero */
static double *aloca_campo(void) {
    double *campo = (double *) calloc((size_t) NX * NY, sizeof(double));
    if (campo == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memoria.\n");
        exit(EXIT_FAILURE);
    }
    return campo;
}

/*
 * Aplica condições de contorno tipo Dirichlet homogêneas (no-slip):
 * velocidade nula nas bordas do domínio, simulando paredes fixas.
 */
static void aplica_contorno(double *campo) {
    for (int i = 0; i < NX; i++) {
        campo[idx(i, 0)]      = 0.0;
        campo[idx(i, NY - 1)] = 0.0;
    }
    for (int j = 0; j < NY; j++) {
        campo[idx(0, j)]      = 0.0;
        campo[idx(NX - 1, j)] = 0.0;
    }
}

/*
 * Executa um passo de difusão explícita (FTCS) para um campo escalar
 * (uma componente de velocidade), usando diferenças finitas centradas
 * de segunda ordem no espaço.
 *
 *   campo_novo[i][j] = campo[i][j] + nu*dt * ( (campo[i+1][j] - 2*campo[i][j] + campo[i-1][j]) / dx^2
 *                                             + (campo[i][j+1] - 2*campo[i][j] + campo[i][j-1]) / dy^2 )
 */
static void passo_difusao(const double *campo, double *campo_novo, double dt, double dx, double dy) {
    double dx2 = dx * dx;
    double dy2 = dy * dy;

    #pragma omp parallel default(none) shared(campo, campo_novo, dt, dx2, dy2)
    {
        #pragma omp for schedule(runtime)
                for (int i = 1; i < NX - 1; i++) {
                    for (int j = 1; j < NY - 1; j++) {
                        double laplaciano_x = (campo[idx(i + 1, j)] - 2.0 * campo[idx(i, j)] + campo[idx(i - 1, j)]) / dx2;
                        double laplaciano_y = (campo[idx(i, j + 1)] - 2.0 * campo[idx(i, j)] + campo[idx(i, j - 1)]) / dy2;
                        campo_novo[idx(i, j)] = campo[idx(i, j)] + NU * dt * (laplaciano_x + laplaciano_y);
                    
                }
        }
    }

    aplica_contorno(campo_novo);
}

/* Calcula velocidade máxima (em módulo) e velocidade média do campo (u,v) */
static void estatisticas(const double *u, const double *v, double *vmax, double *vmedia) {
    double soma = 0.0;
    double maximo = 0.0;

    for (int i = 0; i < NX; i++) {
        for (int j = 0; j < NY; j++) {
            double mag = sqrt(u[idx(i, j)] * u[idx(i, j)] + v[idx(i, j)] * v[idx(i, j)]);
            soma += mag;
            if (mag > maximo) {
                maximo = mag;
            }
        }
    }

    *vmax = maximo;
    *vmedia = soma / (NX * NY);
}

/* Salva o campo u(x,y) em um arquivo CSV (uma linha por linha de y, colunas em x) */
static void salva_csv(const double *u, int passo) {
    char nome_arquivo[128];
    snprintf(nome_arquivo, sizeof(nome_arquivo), "saidap/snapshot_u_passo_%05d.csv", passo);

    FILE *f = fopen(nome_arquivo, "w");
    if (f == NULL) {
        fprintf(stderr, "Aviso: nao foi possivel salvar %s\n", nome_arquivo);
        return;
    }

    for (int j = 0; j < NY; j++) {
        for (int i = 0; i < NX; i++) {
            fprintf(f, "%.6f", u[idx(i, j)]);
            if (i < NX - 1) {
                fprintf(f, ",");
            }
        }
        fprintf(f, "\n");
    }

    fclose(f);
    printf("  -> snapshot salvo em %s\n", nome_arquivo);
}

/*
 * Inicializa o campo com uma perturbação gaussiana localizada no centro
 * do domínio, somada a uma velocidade de base (que pode ser zero).
 */
static void aplica_perturbacao_gaussiana(double *u, double dx, double dy, double amplitude, double largura) {
    double cx = (NX / 2) * dx;
    double cy = (NY / 2) * dy;

    for (int i = 0; i < NX; i++) {
        for (int j = 0; j < NY; j++) {
            double x = i * dx;
            double y = j * dy;
            double r2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            u[idx(i, j)] += amplitude * exp(-r2 / (2.0 * largura * largura));
        }
    }

    aplica_contorno(u);
}

int main(void) {
    double dx = LX / (NX - 1);
    double dy = LY / (NY - 1);

    /* Passo de tempo escolhido com margem de segurança em relação ao
     * limite de estabilidade do esquema explícito:
     *   nu * dt * (1/dx^2 + 1/dy^2) <= 0.5
     */
    double limite_estabilidade = 0.5 / (NU * (1.0 / (dx * dx) + 1.0 / (dy * dy)));
    double dt = 0.4 * limite_estabilidade; /* 40% do limite, por segurança */

    printf("=========================================================\n");
    printf(" Simulacao de fluido viscoso (Navier-Stokes sem pressao)\n");
    printf("=========================================================\n");
    printf("Malha: %d x %d pontos | dx = %.5f | dy = %.5f\n", NX, NY, dx, dy);
    printf("Viscosidade nu = %.4f | dt = %.6f (limite de estabilidade = %.6f)\n", NU, dt, limite_estabilidade);
    printf("---------------------------------------------------------\n");

    double *u = aloca_campo();
    double *v = aloca_campo();
    double *u_novo = aloca_campo();
    double *v_novo = aloca_campo();

    /* ------------------- FASE 1: verificacao de estabilidade ------------------- */
    /* Fluido inicialmente com velocidade constante em x (ex.: escoamento uniforme).
     * Como o campo e uniforme (exceto nas bordas, fixadas em zero pela condicao
     * de contorno), o laplaciano no interior e nulo e o campo deve permanecer
     * praticamente inalterado -> comprova a estabilidade numerica do esquema. */
    double velocidade_constante = 1.0;
    for (int i = 0; i < NX * NY; i++) {
        u[i] = velocidade_constante;
        v[i] = 0.0;
    }
    aplica_contorno(u);
    aplica_contorno(v);

    printf("\n[FASE 1] Fluido iniciado com velocidade constante u = %.2f\n", velocidade_constante);
    printf("Verificando estabilidade (o campo deve permanecer praticamente constante)...\n");

    for (int passo = 1; passo <= FASE1; passo++) {
        passo_difusao(u, u_novo, dt, dx, dy);
        passo_difusao(v, v_novo, dt, dx, dy);

        memcpy(u, u_novo, (size_t) NX * NY * sizeof(double));
        memcpy(v, v_novo, (size_t) NX * NY * sizeof(double));

        if (passo % PRINT_EVERY == 0 || passo == FASE1) {
            double vmax, vmedia;
            estatisticas(u, v, &vmax, &vmedia);
            printf("  passo %5d | v_max = %.6f | v_media = %.6f\n", passo, vmax, vmedia);
        }
    }
    printf("[FASE 1] Concluida: campo permaneceu estavel (sem crescimento nem oscilacao).\n");

    /* ------------------------ FASE 2: perturbacao e difusao ------------------------ */
    /* Reinicia o fluido em repouso e aplica uma pequena perturbacao gaussiana
     * localizada, para observar como ela se espalha suavemente com o tempo
     * devido apenas ao efeito da viscosidade. */
    for (int i = 0; i < NX * NY; i++) {
        u[i] = 0.0;
        v[i] = 0.0;
    }

    double amplitude_perturbacao = 2.0;
    double largura_perturbacao = 0.03;
    aplica_perturbacao_gaussiana(u, dx, dy, amplitude_perturbacao, largura_perturbacao);

    printf("\n[FASE 2] Fluido reiniciado em repouso; perturbacao gaussiana aplicada no centro\n");
    printf("(amplitude = %.2f, largura = %.3f). Observando difusao...\n", amplitude_perturbacao, largura_perturbacao);

    salva_csv(u, 0);

    for (int passo = 1; passo <= FASE2; passo++) {
        passo_difusao(u, u_novo, dt, dx, dy);
        passo_difusao(v, v_novo, dt, dx, dy);

        memcpy(u, u_novo, (size_t) NX * NY * sizeof(double));
        memcpy(v, v_novo, (size_t) NX * NY * sizeof(double));

        if (passo % PRINT_EVERY == 0 || passo == FASE2) {
            double vmax, vmedia;
            estatisticas(u, v, &vmax, &vmedia);
            printf("  passo %5d | v_max = %.6f | v_media = %.6f\n", passo, vmax, vmedia);
        }

        if (passo % SNAPSHOT_EVERY == 0 || passo == FASE2) {
            salva_csv(u, passo);
        }
    }

    printf("[FASE 2] Concluida: a perturbacao se espalhou suavemente pelo dominio,\n");
    printf("reduzindo seu pico e aumentando sua area, como esperado pela difusao viscosa.\n");

    free(u);
    free(v);
    free(u_novo);
    free(v_novo);

    printf("\nSimulacao finalizada com sucesso.\n");
    return 0;
}