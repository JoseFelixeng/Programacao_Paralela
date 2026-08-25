#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define TAM 100000

int ehPrimo(int num){
    if(num < 2){
        return 0;
    }

    for (int i = 2; i < num ; i++){
        if (num % i == 0){
            return 0;
        }
    }

    return 1;
}

int main(){
    int count = 0;
    
    double inicio = omp_get_wtime();

    #pragma omp parallel for
    for(int i = 2; i < TAM; i++){
        if(ehPrimo(i)){
            count++;
        }
    }

    double fim = omp_get_wtime();

    printf("A quantidade de numeros Primos entre 2 e %d: %d\n", TAM, count);
    printf("Tempo Medido: %f (s) \n", fim - inicio);

    return 0;
}