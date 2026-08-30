#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double medirTempo(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(){
    long long N = 10000000;
    long long pontos_dentro = 0;
    double x,y;

    //Inicializa a semente para gerar numeros aleatorios
    srand((unsigned int)time(NULL));

    double inicio = medirTempo();
    for(long long i = 0; i < N; i++){
        //Gear as coordenadas de x e y entre 0 e 1
        x = (double)rand()/RAND_MAX;
        y = (double)rand()/RAND_MAX;

        if((x*x)+(y*y) <= 1.0){
            pontos_dentro++;
        }
    }
    double fim = medirTempo();

    // A área do quarto de círculo é pi/4. 
    // Logo, pi = 4 * (pontos_dentro / pontos_total)
    double pi_estimado = 4.0 * ((double)pontos_dentro / N);

    printf("Tempo Medido: %f (s) \n", fim - inicio);
    printf("Pontos sorteados: %lld\n", N);
    printf("Pontos dentro do círculo: %lld\n", pontos_dentro);
    printf("Valor estimado de Pi: %f\n", pi_estimado);

    return 0;
}