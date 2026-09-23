#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

/*
 * Difusao de calor 1D com MPI -- comparacao de tres estrategias de
 * comunicacao para a troca de bordas (halo exchange):
 *
 *   1 = MPI_Send / MPI_Recv          (bloqueante)
 *   2 = MPI_Isend / MPI_Irecv + MPI_Wait   (nao-bloqueante, sem overlap)
 *   3 = MPI_Isend / MPI_Irecv + MPI_Test   (nao-bloqueante, COM overlap:
 *       atualiza os pontos internos em pedacos, testando a comunicacao
 *       a cada pedaco, e so calcula os pontos de borda quando a
 *       comunicacao estiver garantidamente completa)
 */

#define N_GLOBAL_DEFAULT 1000000
#define N_STEPS_DEFAULT  1000
#define ALPHA            0.25
#define CHUNK_SIZE       2000   /* tamanho do pedaco computado entre cada MPI_Test */

#define TAG_R 0   /* mensagem "andando para a direita" (enviada ao vizinho direito) */
#define TAG_L 1   /* mensagem "andando para a esquerda" (enviada ao vizinho esquerdo) */

static void atualizar_pontos(const double *u_old, double *u_new,
                              int start, int end, double c)
{
    for (int i = start; i <= end; i++)
        u_new[i] = u_old[i] + c * (u_old[i + 1] - 2.0 * u_old[i] + u_old[i - 1]);
}

int main(int argc, char **argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int mode      = (argc > 1) ? atoi(argv[1]) : 3;
    int N_GLOBAL  = (argc > 2) ? atoi(argv[2]) : N_GLOBAL_DEFAULT;
    int N_STEPS   = (argc > 3) ? atoi(argv[3]) : N_STEPS_DEFAULT;
    if (mode < 1 || mode > 3) mode = 3;

    /* Distribuicao uniforme com resto repartido entre os primeiros processos,
     * para nao perder pontos quando N_GLOBAL nao e multiplo de size.        */
    int base  = N_GLOBAL / size;
    int resto = N_GLOBAL % size;
    int n_local  = base + (rank < resto ? 1 : 0);
    int offset   = (rank < resto) ? rank * (base + 1)
                                   : resto * (base + 1) + (rank - resto) * base;

    double *u_old = calloc(n_local + 2, sizeof(double));
    double *u_new = calloc(n_local + 2, sizeof(double));
    if (!u_old || !u_new) { MPI_Abort(MPI_COMM_WORLD, 1); }

    /* Condicao inicial: pulso de calor no terco central do dominio global */
    for (int i = 1; i <= n_local; i++) {
        int gidx = offset + (i - 1);
        u_old[i] = (gidx >= N_GLOBAL / 3 && gidx <= 2 * N_GLOBAL / 3) ? 100.0 : 0.0;
    }

    int left  = (rank == 0)        ? MPI_PROC_NULL : rank - 1;
    int right = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

    if (rank == 0) {
        const char *nomes[] = {"", "Send/Recv (bloqueante)",
                                "Isend/Irecv + Wait",
                                "Isend/Irecv + Test (overlap)"};
        printf("Modo %d: %s | N=%d P=%d passos=%d\n",
               mode, nomes[mode], N_GLOBAL, size, N_STEPS);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    for (int step = 0; step < N_STEPS; step++) {

        if (mode == 1) {
            /* ---------------------------------------------------------------
             * MODO 1: MPI_Send / MPI_Recv bloqueantes.
             * A ordem send/recv e escalonada por paridade do rank para evitar
             * deadlock (todo processo enviando ao mesmo tempo pode travar,
             * dependendo do protocolo/tamanho da mensagem no MPI usado).
             * --------------------------------------------------------------- */
            if (rank % 2 == 0) {
                MPI_Send(&u_old[n_local], 1, MPI_DOUBLE, right, TAG_R, MPI_COMM_WORLD);
                MPI_Recv(&u_old[0],       1, MPI_DOUBLE, left,  TAG_R, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(&u_old[1],       1, MPI_DOUBLE, left,  TAG_L, MPI_COMM_WORLD);
                MPI_Recv(&u_old[n_local+1], 1, MPI_DOUBLE, right, TAG_L, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(&u_old[0],       1, MPI_DOUBLE, left,  TAG_R, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(&u_old[n_local], 1, MPI_DOUBLE, right, TAG_R, MPI_COMM_WORLD);
                MPI_Recv(&u_old[n_local+1], 1, MPI_DOUBLE, right, TAG_L, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(&u_old[1],       1, MPI_DOUBLE, left,  TAG_L, MPI_COMM_WORLD);
            }

            atualizar_pontos(u_old, u_new, 1, n_local, ALPHA);

        } else if (mode == 2) {
            /* ---------------------------------------------------------------
             * MODO 2: MPI_Isend / MPI_Irecv seguido de MPI_Wait imediato.
             * Nao ha sobreposicao: a CPU fica ociosa esperando a comunicacao
             * terminar antes de comecar a computar qualquer ponto.
             * --------------------------------------------------------------- */
            MPI_Request reqs[4];
            MPI_Irecv(&u_old[0],         1, MPI_DOUBLE, left,  TAG_R, MPI_COMM_WORLD, &reqs[0]);
            MPI_Irecv(&u_old[n_local+1], 1, MPI_DOUBLE, right, TAG_L, MPI_COMM_WORLD, &reqs[1]);
            MPI_Isend(&u_old[n_local],   1, MPI_DOUBLE, right, TAG_R, MPI_COMM_WORLD, &reqs[2]);
            MPI_Isend(&u_old[1],         1, MPI_DOUBLE, left,  TAG_L, MPI_COMM_WORLD, &reqs[3]);

            MPI_Wait(&reqs[0], MPI_STATUS_IGNORE);
            MPI_Wait(&reqs[1], MPI_STATUS_IGNORE);
            MPI_Wait(&reqs[2], MPI_STATUS_IGNORE);
            MPI_Wait(&reqs[3], MPI_STATUS_IGNORE);

            atualizar_pontos(u_old, u_new, 1, n_local, ALPHA);

        } else {
            /* ---------------------------------------------------------------
             * MODO 3: MPI_Isend / MPI_Irecv + MPI_Test em loop.
             * Enquanto a comunicacao das bordas esta em transito, calculamos
             * os pontos INTERNOS (que nao dependem das ghost cells) em
             * pedacos de CHUNK_SIZE, testando a cada pedaco se a comunicacao
             * ja terminou. So os pontos de BORDA, que dependem das ghost
             * cells recebidas, esperam a confirmacao (MPI_Test/Waitall) de
             * que a comunicacao de fato concluiu.
             * --------------------------------------------------------------- */
            MPI_Request reqs[4];
            MPI_Irecv(&u_old[0],         1, MPI_DOUBLE, left,  TAG_R, MPI_COMM_WORLD, &reqs[0]);
            MPI_Irecv(&u_old[n_local+1], 1, MPI_DOUBLE, right, TAG_L, MPI_COMM_WORLD, &reqs[1]);
            MPI_Isend(&u_old[n_local],   1, MPI_DOUBLE, right, TAG_R, MPI_COMM_WORLD, &reqs[2]);
            MPI_Isend(&u_old[1],         1, MPI_DOUBLE, left,  TAG_L, MPI_COMM_WORLD, &reqs[3]);

            int interior_end = n_local - 1;   /* ultimo ponto interno (nao depende de ghost) */
            int next = 2;                     /* primeiro ponto interno */
            int comm_done = 0;

            while (next <= interior_end) {
                int chunk_end = next + CHUNK_SIZE - 1;
                if (chunk_end > interior_end) chunk_end = interior_end;

                atualizar_pontos(u_old, u_new, next, chunk_end, ALPHA);
                next = chunk_end + 1;

                MPI_Testall(4, reqs, &comm_done, MPI_STATUSES_IGNORE);
            }

            /* Garante que a comunicacao terminou antes de tocar nas ghost cells */
            if (!comm_done)
                MPI_Waitall(4, reqs, MPI_STATUSES_IGNORE);

            /* Pontos de borda, que dependem das ghost cells recem-recebidas */
            atualizar_pontos(u_old, u_new, 1, 1, ALPHA);
            atualizar_pontos(u_old, u_new, n_local, n_local, ALPHA);
        }

        double *tmp = u_old; u_old = u_new; u_new = tmp;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t1 = MPI_Wtime();
    double t_local = t1 - t0, t_max;
    MPI_Reduce(&t_local, &t_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    double soma_local = 0.0;
    for (int i = 1; i <= n_local; i++) soma_local += u_old[i];
    double soma_global;
    MPI_Reduce(&soma_local, &soma_global, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
        printf("  -> tempo=%.6f s | soma_global=%.6f\n", t_max, soma_global);

    free(u_old);
    free(u_new);
    MPI_Finalize();
    return 0;
}
