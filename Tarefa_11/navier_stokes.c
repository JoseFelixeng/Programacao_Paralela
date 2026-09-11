/*
 * ============================================================================
 * Simulação da equação de Navier-Stokes considerando APENAS viscosidade
 * ============================================================================
 *
 * Equação de Navier-Stokes completa (incompressível):
 *
 *   du/dt + (u . grad)u = -grad(p)/rho + nu * laplaciano(u) + f
 *
 * Neste programa desconsideramos:
 *   - o termo advectivo (u . grad)u
 *   - o gradiente de pressão (-grad(p)/rho)
 *   - forças externas (f)
 *
 * Restando apenas o termo de difusão viscosa:
 *
 *   du/dt = nu * laplaciano(u)
 *   dv/dt = nu * laplaciano(v)
 *
 * que é exatamente a equação do calor (difusão) aplicada a cada componente
 * do campo de velocidade (u, v). Discretizamos o espaço com diferenças
 * finitas centradas de 2ª ordem em uma malha 2D com condições de contorno
 * periódicas, e o tempo com Euler explícito.
 *
 * Estrutura do programa:
 *   1. Fluido parado (u=v=0)      -> verifica que o campo permanece estável
 *   2. Fluido com velocidade const -> verifica que o campo permanece estável
 *   3. Perturbação gaussiana local -> observa a difusão suave no tempo
 *
 * Compilação:
 *   gcc -O2 -Wall -o navier_stokes_viscosidade navier_stokes_viscosidade.c -lm
 *
 * Execução:
 *   ./navier_stokes_viscosidade
 *
 * Saída:
 *   - Estatísticas impressas no terminal (max|u|, energia cinética)
 *   - Arquivos CSV "snapshot_XXXX.csv" com o campo u(x,y) em instantes
 *     selecionados da fase de perturbação, para visualização posterior
 *     (ex.: Python + matplotlib.imshow / heatmap).
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* ---------------------------- Parâmetros ---------------------------------*/

#define NX 100          /* pontos da malha em x */
#define NY 100          /* pontos da malha em y */
#define DX 1.0          /* espaçamento da malha em x */
#define DY 1.0          /* espaçamento da malha em y */
#define NU 0.1          /* viscosidade cinemática */

/* Número de passos de tempo em cada fase */
#define STEPS_ESTAVEL      500   /* fases 1 e 2 (verificação de estabilidade) */
#define STEPS_PERTURBACAO 2000   /* fase 3 (difusão da perturbação) */

/* A cada quantos passos imprimir estatísticas / salvar snapshot */
#define INTERVALO_LOG        50
#define INTERVALO_SNAPSHOT  200

/* Fator de segurança sobre o limite de estabilidade (0 < FATOR_CFL <= 1) */
#define FATOR_CFL 0.5

/* ---------------------------------------------------------------------- */

typedef struct {
    double u[NX][NY];   /* componente x da velocidade */
    double v[NX][NY];   /* componente y da velocidade */
} Campo;

/* Índice periódico (condição de contorno periódica) */
static inline int idx_periodico(int i, int n) {
    if (i < 0) return i + n;
    if (i >= n) return i - n;
    return i;
}

/* Inicializa o fluido parado: u = v = 0 em todo o domínio */
void inicializar_parado(Campo *c) {
    memset(c->u, 0, sizeof(c->u));
    memset(c->v, 0, sizeof(c->v));
}

/* Inicializa o fluido com velocidade constante (escoamento uniforme) */
void inicializar_constante(Campo *c, double u0, double v0) {
    for (int i = 0; i < NX; i++)
        for (int j = 0; j < NY; j++) {
            c->u[i][j] = u0;
            c->v[i][j] = v0;
        }
}

/*
 * Adiciona uma perturbação gaussiana localizada ao campo de velocidade
 * (soma-se ao valor já existente, para poder perturbar um fluido em
 * repouso ou em movimento uniforme).
 *
 * amplitude: intensidade máxima da perturbação
 * largura:   desvio padrão (sigma) da gaussiana, em pontos de malha
 */
void adicionar_perturbacao(Campo *c, double amplitude, double largura) {
    int cx = NX / 2;
    int cy = NY / 2;

    for (int i = 0; i < NX; i++) {
        for (int j = 0; j < NY; j++) {
            double dx = i - cx;
            double dy = j - cy;
            double r2 = dx * dx + dy * dy;
            double gauss = amplitude * exp(-r2 / (2.0 * largura * largura));
            c->u[i][j] += gauss;
            /* v permanece sem perturbação, apenas u recebe o pulso */
        }
    }
}

/*
 * Executa um passo de tempo da difusão viscosa pura, usando diferenças
 * finitas centradas (5 pontos) para o laplaciano e Euler explícito no
 * tempo. Trabalha com buffers separados (entrada/saída) para evitar
 * usar valores já atualizados no mesmo passo (esquema tipo Jacobi).
 *
 *   u_new[i][j] = u[i][j] + nu*dt * [ (u[i+1][j] - 2u[i][j] + u[i-1][j]) / dx^2
 *                                    + (u[i][j+1] - 2u[i][j] + u[i][j-1]) / dy^2 ]
 */
void passo_difusao(const Campo *atual, Campo *proximo, double dt) {
    double rx = NU * dt / (DX * DX);
    double ry = NU * dt / (DY * DY);

    for (int i = 0; i < NX; i++) {
        int ip = idx_periodico(i + 1, NX);
        int im = idx_periodico(i - 1, NX);
        for (int j = 0; j < NY; j++) {
            int jp = idx_periodico(j + 1, NY);
            int jm = idx_periodico(j - 1, NY);

            double lap_u = (atual->u[ip][j] - 2.0 * atual->u[i][j] + atual->u[im][j]) * rx
                         + (atual->u[i][jp] - 2.0 * atual->u[i][j] + atual->u[i][jm]) * ry;

            double lap_v = (atual->v[ip][j] - 2.0 * atual->v[i][j] + atual->v[im][j]) * rx
                         + (atual->v[i][jp] - 2.0 * atual->v[i][j] + atual->v[i][jm]) * ry;

            proximo->u[i][j] = atual->u[i][j] + lap_u;
            proximo->v[i][j] = atual->v[i][j] + lap_v;
        }
    }
}

/* Retorna a magnitude máxima da velocidade |V| = sqrt(u^2+v^2) no domínio */
double velocidade_maxima(const Campo *c) {
    double max_v = 0.0;
    for (int i = 0; i < NX; i++)
        for (int j = 0; j < NY; j++) {
            double mag = sqrt(c->u[i][j] * c->u[i][j] + c->v[i][j] * c->v[i][j]);
            if (mag > max_v) max_v = mag;
        }
    return max_v;
}

/* Energia cinética total (proporcional a soma de u^2+v^2) */
double energia_cinetica(const Campo *c) {
    double energia = 0.0;
    for (int i = 0; i < NX; i++)
        for (int j = 0; j < NY; j++)
            energia += 0.5 * (c->u[i][j] * c->u[i][j] + c->v[i][j] * c->v[i][j]);
    return energia;
}

/* Salva o campo u(x,y) em um arquivo CSV para visualização externa */
void salvar_snapshot(const Campo *c, const char *nome_arquivo) {
    FILE *f = fopen(nome_arquivo, "w");
    if (!f) {
        fprintf(stderr, "Erro ao abrir arquivo %s para escrita\n", nome_arquivo);
        return;
    }
    for (int i = 0; i < NX; i++) {
        for (int j = 0; j < NY; j++) {
            fprintf(f, "%.6f", c->u[i][j]);
            if (j < NY - 1) fprintf(f, ",");
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

/*
 * Executa uma fase de simulação (estável ou com perturbação), fazendo
 * a troca de buffers a cada passo e imprimindo/gravando estatísticas
 * periodicamente.
 */
void simular_fase(Campo *campo, int n_passos, double dt,
                   int salvar_snapshots, const char *prefixo) {
    Campo *buffer_a = campo;
    Campo *buffer_b = malloc(sizeof(Campo));
    if (!buffer_b) {
        fprintf(stderr, "Erro de alocação de memória\n");
        exit(1);
    }
    memcpy(buffer_b, buffer_a, sizeof(Campo));

    for (int passo = 0; passo <= n_passos; passo++) {
        if (passo % INTERVALO_LOG == 0) {
            double vmax = velocidade_maxima(buffer_a);
            double energia = energia_cinetica(buffer_a);
            printf("  passo %5d  |  t = %8.3f  |  max|V| = %10.6f  |  energia = %12.6f\n",
                   passo, passo * dt, vmax, energia);
        }

        if (salvar_snapshots && passo % INTERVALO_SNAPSHOT == 0) {
            char nome[128];
            snprintf(nome, sizeof(nome), "saida/%s_%04d.csv", prefixo, passo);
            salvar_snapshot(buffer_a, nome);
        }

        if (passo == n_passos) break;

        passo_difusao(buffer_a, buffer_b, dt);

        /* troca de buffers (swap de ponteiros) */
        Campo *tmp = buffer_a;
        buffer_a = buffer_b;
        buffer_b = tmp;
    }

    /* garante que o resultado final fique no campo original passado pelo chamador */
    if (buffer_a != campo) memcpy(campo, buffer_a, sizeof(Campo));

    free(buffer_b == campo ? buffer_a : buffer_b);
}

int main(void) {
    /* --- Passo de tempo escolhido a partir do critério de estabilidade ---
     * Para o esquema explícito de difusão 2D, a estabilidade exige:
     *     nu * dt * (1/dx^2 + 1/dy^2) <= 0.5
     * Aplicamos um fator de segurança (FATOR_CFL) sobre esse limite.
     */
    double dt_limite = 0.5 / (NU * (1.0 / (DX * DX) + 1.0 / (DY * DY)));
    double dt = FATOR_CFL * dt_limite;

    printf("============================================================\n");
    printf(" -------------Simulacao de Navier-Stokes-------------------\n");
    printf("============================================================\n");
    printf("Malha: %d x %d | dx=dy= %.2f | nu= %.4f\n", NX, NY, DX, NU);
    printf("dt escolhido = %.6f (limite de estabilidade = %.6f, fator = %.2f)\n\n", dt, dt_limite, FATOR_CFL);

    Campo campo;

    /* ---------------- FASE 1: fluido parado ---------------- */
    printf("---- FASE 1: fluido inicialmente PARADO (u=v=0) ----\n");
    inicializar_parado(&campo);
    simular_fase(&campo, STEPS_ESTAVEL, dt, 0, NULL);
    printf("-> Esperado: max|V| e energia permanecem em 0 (campo trivialmente estavel).\n\n");

    /* ---------------- FASE 2: velocidade constante ---------------- */
    printf("---- FASE 2: fluido com velocidade CONSTANTE (u0=1.0, v0=0.5) ----\n");
    inicializar_constante(&campo, 1.0, 0.5);
    simular_fase(&campo, STEPS_ESTAVEL, dt, 0, NULL);
    printf("-> Esperado: max|V| e energia permanecem constantes,\n");
    printf("   pois o laplaciano de um campo uniforme e nulo.\n\n");

    /* ---------------- FASE 3: perturbação local ---------------- */
    printf("---- FASE 3: PERTURBACAO gaussiana em fluido parado ----\n");
    inicializar_parado(&campo);
    adicionar_perturbacao(&campo, /*amplitude=*/5.0, /*largura=*/3.0);
    printf("Perturbacao inicial: gaussiana no centro da malha, amplitude=5.0, sigma=3.0\n");
    printf("Snapshots do campo u(x,y) serao salvos em /mnt/user-data/outputs/\n\n");
    simular_fase(&campo, STEPS_PERTURBACAO, dt, 1, "snapshot");
    printf("\n-> Esperado: max|V| decai suavemente com o tempo e a energia se conserva\n");
    printf("   aproximadamente no inicio, dissipando lentamente (difusao pura nao\n");
    printf("   conserva energia perfeitamente, pois viscosidade dissipa energia\n");
    printf("   cinetica em calor). O pico se espalha (alarga) e se achata, sem\n");
    printf("   oscilacoes bruscas -- comportamento tipico da equacao do calor.\n");

    return 0;
}