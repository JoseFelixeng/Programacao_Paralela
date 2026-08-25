#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAM 100000

static double medirTempo(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

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
    
    double inicio = medirTempo();
    for(int i = 2; i < TAM; i++){
        if(ehPrimo(i)){
            count++;
        }
    }
    double fim = medirTempo();

    printf("A quantidade de numeros Primos entre 2 e %d: %d\n", TAM, count);
    printf("Tempo Medido: %f (s) \n", fim - inicio);

    return 0;
}