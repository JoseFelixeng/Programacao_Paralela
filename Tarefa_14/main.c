#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ITERACOES 1000

int main(int argc, char** argv) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Garante que o programa é executado com exatamente 2 processos
    if (size != 2) {
        if (rank == 0) {
            printf("Erro: A Tarefa 14 exige exatamente 2 processos!\n");
        }
        MPI_Finalize();
        return 0;
    }

    // Tamanho máximo de 8 MB
    long max_bytes = 8 * 1024 * 1024; 
    char* buffer = (char*) malloc(max_bytes);

    if (rank == 0) {
        printf("| Tamanho_Bytes   |  Tempo_Medio_Msg_s    |    Largura_Banda_MBs  | \n");
    }

    // Variando o tamanho da mensagem (de 8 bytes até 8 MB)
    for (long bytes = 8; bytes <= max_bytes; bytes *= 2) {
        
        // Sincroniza os dois processos antes de iniciar a medição do cronômetro
        MPI_Barrier(MPI_COMM_WORLD);
        
        double start = MPI_Wtime(); // Iniciar a medição

        for (int i = 0; i < ITERACOES; i++) {
            if (rank == 0) {
                MPI_Send(buffer, bytes, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(buffer, bytes, MPI_BYTE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (rank == 1) {
                MPI_Recv(buffer, bytes, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buffer, bytes, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
            }
        }

        double finish = MPI_Wtime(); // Finalizar a medição

        if (rank == 0) {
            double tempo_total = finish - start;
            // Cada iteração do laço realiza 2 mensagens (uma ida e uma volta)
            double tempo_uma_mensagem = tempo_total / (2.0 * ITERACOES); 
            double bytes_para_MB = bytes / (1024.0 * 1024.0);
            double largura_banda = bytes_para_MB / tempo_uma_mensagem;

            printf("|   %ld |    %e|   %f   |\n", bytes, tempo_uma_mensagem, largura_banda);
        }
    }

    free(buffer);
    MPI_Finalize();
    return 0;
}
