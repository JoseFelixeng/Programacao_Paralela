/*
 * Tarefa 16 - Multiplicacao matriz-vetor distribuida com MPI
 * y = A * x,  A: M x N (row-major),  x: N,  y: M
 *
 * Estrategia:
 *   - A matriz A e distribuida por linhas entre os processos com MPI_Scatter
 *     (cada processo recebe local_M = M / nprocs linhas contiguas)
 *   - O vetor x inteiro e distribuido a todos com MPI_Bcast
 *   - Cada processo calcula os elementos de y correspondentes as suas linhas
 *   - Os resultados parciais sao recolhidos no processo 0 com MPI_Gather
 *
 * Uso:
 *   mpirun -np P ./mv_mpi M N [reps] [--verify]
 *
 *   M, N     : dimensoes da matriz (M deve ser divisivel por P, pois
 *              MPI_Scatter exige blocos de tamanho igual)
 *   reps     : numero de repeticoes usadas para medir o tempo medio
 *              (default = 10)
 *   --verify : depois das medicoes, o processo 0 recalcula y de forma
 *              serial e imprime o maior erro absoluto em relacao ao
 *              resultado paralelo, para validar a corretude
 */

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

    if (M % nprocs != 0) {
        if (rank == 0) {
            fprintf(stderr,
                "Erro: M=%ld precisa ser divisivel pelo numero de "
                "processos (nprocs=%d) para usar MPI_Scatter/MPI_Gather "
                "com blocos de tamanho igual.\n", M, nprocs);
        }
        MPI_Finalize();
        return 1;
    }

    long local_M = M / nprocs;

    /* Apenas o processo 0 possui a matriz completa e o vetor y completo */
    double *A = NULL, *y = NULL;
    double *x = aloca_vetor(N);              /* todos os processos terao x apos o Bcast */
    double *local_A = aloca_vetor(local_M * N);
    double *local_y = aloca_vetor(local_M);

    if (rank == 0) {
        A = aloca_vetor(M * N);
        y = aloca_vetor(M);
        preenche_aleatorio(A, M * N, 42u);
        preenche_aleatorio(x, N, 7u);
    }

    double tempo_total = 0.0;

    for (int r = 0; r < reps; r++) {
        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();

        MPI_Scatter(A, (int)(local_M * N), MPI_DOUBLE,
                    local_A, (int)(local_M * N), MPI_DOUBLE,
                    0, MPI_COMM_WORLD);

        MPI_Bcast(x, (int) N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        for (long i = 0; i < local_M; i++) {
            double soma = 0.0;
            const double *linha = local_A + i * N;
            for (long j = 0; j < N; j++) {
                soma += linha[j] * x[j];
            }
            local_y[i] = soma;
        }

        MPI_Gather(local_y, (int) local_M, MPI_DOUBLE,
                   y, (int) local_M, MPI_DOUBLE,
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
            fprintf(stderr,
                "[verify] erro absoluto maximo entre paralelo e serial: %e\n",
                erro_max);
            free(y_serial);
        }
    }

    free(x);
    free(local_A);
    free(local_y);
    if (rank == 0) {
        free(A);
        free(y);
    }

    MPI_Finalize();
    return 0;
}