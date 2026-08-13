// Online C compiler to run C program online
#include <stdio.h>
#include <math.h>
#include <time.h>

#define _USE_MATH_DEFINES

long double calcularPiLeibniz(int interacao) {
    long double pi = 0.0L; // 0.0L serve para que o compilador trate como o argumento long double
    long double termo;
    int i;
    for (i = 0; i < interacao; i++) {
        if (i % 2 == 0) {
            // Somar apenas os termos positivos
            termo = 1.0 / (2 * i + 1);
        } else {
            // Se o termo for impar
            termo = -1.0 / (2 * i + 1);
        }
        pi = pi + termo;
    }
    return pi * 4.0L;
}

int main(void) {
    struct timespec inicioIteracao, fimIteracao;
    struct timespec inicioTotal, fimTotal;
    const double pi_real = M_PI;
    int listaDeIteracoes[] = {10, 100, 1000, 10000, 100000, 300000, 500000, 750000, 950000, 1000000};
    int n = sizeof(listaDeIteracoes) / sizeof(listaDeIteracoes[0]);
    int j;

    printf("%-15s %-22s %-18s %-15s\n",
           "Iteracoes", "Pi aproximado", "Erro absoluto", "Walltime (s)");
    printf("--------------------------------------------------------------------\n");

    clock_gettime(CLOCK_MONOTONIC, &inicioTotal);

    for (j = 0; j < n; j++) {
        int interacao = listaDeIteracoes[j];

        // MARCA O TEMPO SO EM VOLTA DA CHAMADA QUE QUEREMOS MEDIR
        clock_gettime(CLOCK_MONOTONIC, &inicioIteracao);
        long double piCalculado = calcularPiLeibniz(interacao);
        clock_gettime(CLOCK_MONOTONIC, &fimIteracao);

        long double erro = fabsl(pi_real - piCalculado);
        double tempoIteracao = (fimIteracao.tv_sec - inicioIteracao.tv_sec) +
                                (fimIteracao.tv_nsec - inicioIteracao.tv_nsec) / 1e9;

        printf("%-15d %-22.15Lf %-18.15Lf %-15.6f\n",
               interacao, piCalculado, erro, tempoIteracao);
    }

    clock_gettime(CLOCK_MONOTONIC, &fimTotal);

    double tempoTotal = (fimTotal.tv_sec - inicioTotal.tv_sec) +
                         (fimTotal.tv_nsec - inicioTotal.tv_nsec) / 1e9;

    printf("--------------------------------------------------------------------\n");
    printf("Tempo total (walltime) para todas as iteracoes: %.6f s\n", tempoTotal);

    return 0;
}