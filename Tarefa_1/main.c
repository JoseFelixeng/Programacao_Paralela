// Online C compiler to run C program online
#include <stdio.h>
#include <math.h>
#include <time.h>

#define _USE_MATH_DEFINES


long double calcularPiLeibniz(int interacao){// usada para calcular o tempo
    long double pi = 0.0L; // 0.0L serve para que o compilador trate como o argumento long double
    long double termo;
    int i; 
    for(i = 0; i < interacao; i++){
        if(i % 2 == 0 ){
            // Somar apenas os termos positivos
            termo = 1.0/( 2 * i + 1);
        }else{
            // Se o termo for Impar 
            termo = -1.0/( 2 * i + 1);
        };
        pi = pi + termo;
    }
    return pi * 4.0L; 
}



int main(){
    struct timespec inicio, fim; 
    const double pi_real = M_PI;
    int listaDeIteracoes[] = {10, 100, 1000, 10000, 100000, 300000, 500000, 750000, 950000, 1000000};
    int n = sizeof(listaDeIteracoes) / sizeof(listaDeIteracoes[0]);
    int j; 

    printf("%-15s %-22s %-18s %-15s\n",
           "Iteracoes", "Pi aproximado", "Erro absoluto", "Walltime (s)");
    printf("--------------------------------------------------------------------\n");

    clock_gettime(CLOCK_MONOTONIC, &inicio);
    
    for(j=0; j < n; j++){
        int interacao = listaDeIteracoes[j];
        long double piCalculado = calcularPiLeibniz(interacao);
        long double erro = fabsl(pi_real - piCalculado);
        printf("%-15d %-22.15Lf %-18.15Lf\n", interacao, piCalculado, erro);

    }

    clock_gettime(CLOCK_MONOTONIC, &fim);

    // Calcular o tempo de execução 
    double tempoT = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec)/1e9;

    
    printf("--------------------------------------------------------------------\n");
    printf("Tempo total (walltime) para todas as iteracoes: %.6f s\n", tempoT);


    return 0;

};