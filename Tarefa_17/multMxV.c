#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static double *aloca_vetor(long n) {
    double *v = (double *) malloc((size_t) n * sizeof(double));
    if (!v) {
        fprintf(stderr, "Falha ao alocar %ld doubles\n", n);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    return v;
}

/* Gera valores pseudo-aleatorios deterministicos (mesma semente sempre),
 * para que as execucoes sejam reprodutiveis e comparaveis entre si. */
static void preenche_aleatorio(double *v, long n, unsigned int seed) {
    unsigned int state = seed;
    for (long i = 0; i < n; i++) {
        state = state * 1103515245u + 12345u;
        v[i] = (double) ((state >> 8) % 1000) / 100.0; /* [0, 10) */
    }
}

static void multiplica_serial(const double *A, const double *x, double *y,
                               long M, long N) {
    for (long i = 0; i < M; i++) {
        double soma = 0.0;
        const double *linha = A + i * N;
        for (long j = 0; j < N; j++) {
            soma += linha[j] * x[j];
        }
        y[i] = soma;
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc < 3) {
        if (rank == 0) {
            fprintf(stderr,
                "Uso: %s M N [reps] [--verify]\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    long M = atol(argv[1]);
    long N = atol(argv[2]);
    int reps = (argc >= 4 && argv[3][0] != '-') ? atoi(argv[3]) : 10;
    int verify = 0;
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--verify") == 0) verify = 1;
    }

    /* Agora quem precisa ser divisivel pelo numero de processos e o
     * numero de COLUNAS (N), pois e ela que sera particionada entre
     * os processos com MPI_Scatter + tipo derivado. */
    if (N % nprocs != 0) {
        if (rank == 0) {
            fprintf(stderr, "Erro: N=%ld precisa ser divisivel pelo numero de " "processos (nprocs=%d) para usar MPI_Scatter com blocos de " "colunas de tamanho igual.\n", N, nprocs);
        }
        MPI_Finalize();
        return 1;
    }

    long local_N = N / nprocs;

    /* ---------------- Tipo derivado: uma coluna de A ---------------- */
    /* A e M x N, armazenada em row-major (A[i*N+j]). Uma coluna j tem
     * M elementos, cada um a N posicoes (doubles) de distancia do
     * anterior: MPI_Type_vector(M blocos, 1 elemento por bloco, stride N). */
    MPI_Datatype coluna_tmp, tipo_coluna;
    MPI_Type_vector((int) M, 1, (int) N, MPI_DOUBLE, &coluna_tmp);

    /* O extent natural desse tipo vai do 1o ao ultimo elemento da coluna
     * (quase a matriz inteira), o que faria o MPI_Scatter posicionar
     * cada "unidade" de coluna muito distante da seguinte. Redefinindo
     * o extent para 1 double, count=local_N desse tipo redimensionado
     * passa a corresponder a local_N colunas ADJACENTES (col 0,1,2,...),
     * cada uma comecando logo apos o inicio da anterior. */
    MPI_Type_create_resized(coluna_tmp, 0, sizeof(double), &tipo_coluna);
    MPI_Type_commit(&tipo_coluna);
    MPI_Type_free(&coluna_tmp);

    /* Apenas o processo 0 possui a matriz completa e o vetor y completo */
    double *A = NULL, *x = NULL, *y = NULL;
    double *local_x = aloca_vetor(local_N);
    /* local_A guarda local_N colunas de M elementos cada, entregues pelo
     * MPI_Scatter de forma contigua por coluna (column-major local):
     * local_A[jl*M + i] = A[i*N + (rank*local_N + jl)]. */
    double *local_A = aloca_vetor(local_N * M);
    double *local_y_parcial = aloca_vetor(M);

    if (rank == 0) {
        A = aloca_vetor(M * N);
        x = aloca_vetor(N);
        y = aloca_vetor(M);
        preenche_aleatorio(A, M * N, 42u);
        preenche_aleatorio(x, N, 7u);
    }

    double tempo_total = 0.0;

    for (int r = 0; r < reps; r++) {
        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();

        /* Distribui blocos de local_N colunas por processo */
        MPI_Scatter(A, (int) local_N, tipo_coluna,
                    local_A, (int) (local_N * M), MPI_DOUBLE,
                    0, MPI_COMM_WORLD);

        /* Distribui os segmentos de x correspondentes as colunas de cada
         * processo (contiguo em x, dispensa tipo derivado). */
        MPI_Scatter(x, (int) local_N, MPI_DOUBLE,
                    local_x, (int) local_N, MPI_DOUBLE,
                    0, MPI_COMM_WORLD);

        /* Cada processo contribui para TODOS os M elementos de y:
         *   y_parcial[i] += soma_{j local} A[i][j] * x[j]           */
        memset(local_y_parcial, 0, (size_t) M * sizeof(double));
        for (long jl = 0; jl < local_N; jl++) {
            double xj = local_x[jl];
            const double *coluna = local_A + jl * M;
            for (long i = 0; i < M; i++) {
                local_y_parcial[i] += coluna[i] * xj;
            }
        }

        /* Soma elemento a elemento as contribuicoes parciais de todos os
         * processos, entregando o y completo (tamanho M) no processo 0. */
        MPI_Reduce(local_y_parcial, y, (int) M, MPI_DOUBLE, MPI_SUM,
                   0, MPI_COMM_WORLD);

        MPI_Barrier(MPI_COMM_WORLD);
        double t1 = MPI_Wtime();
        tempo_total += (t1 - t0);
    }

    if (rank == 0) {
        double tempo_medio = tempo_total / reps;
        /* Saida em CSV para facilitar coleta de dados nos benchmarks:
         * nprocs,M,N,reps,tempo_medio_s */
        printf("%d,%ld,%ld,%d,%.9f\n", nprocs, M, N, reps, tempo_medio);

        if (verify) {
            double *y_serial = aloca_vetor(M);
            multiplica_serial(A, x, y_serial, M, N);
            double erro_max = 0.0;
            for (long i = 0; i < M; i++) {
                double dif = fabs(y[i] - y_serial[i]);
                if (dif > erro_max) erro_max = dif;
            }
            fprintf(stderr, "[verify] erro absoluto maximo entre paralelo e serial: %e\n", erro_max);
            free(y_serial);
        }
    }

    MPI_Type_free(&tipo_coluna);

    free(local_x);
    free(local_A);
    free(local_y_parcial);
    if (rank == 0) {
        free(A);
        free(x);
        free(y);
    }

    MPI_Finalize();
    return 0;
}