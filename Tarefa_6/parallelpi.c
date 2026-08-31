#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

int main(){
    long long N = 10000000;
    long long pontos_dentro = 0;
    int offset = 1; 
    long long ultimo_i = -1;


    double inicio = omp_get_wtime();
   #pragma omp parallel default(none) shared(N, pontos_dentro) firstprivate(offset) lastprivate(ultimo_i)
   {
        unsigned int seed = omp_get_thread_num() + offset;
        long long local_dento_c = 0;
        double x,y;


        #pragma omp for   
        for(long long i = 0; i < N; i++){
        //Gear as coordenadas de x e y entre 0 e 1
            x = (double)rand_r(&seed)/RAND_MAX;
            y = (double)rand_r(&seed)/RAND_MAX;

            if((x*x)+(y*y) <= 1.0){
                local_dento_c++;
            }
            ultimo_i = i;
        }
        #pragma omp critical 
        pontos_dentro += local_dento_c;
   }

    double fim = omp_get_wtime();

    double pi_estimado = 4.0 * ((double)pontos_dentro / N);

    printf("Pontos sorteados: %lld\n", N);
    printf("Pontos dentro do círculo: %lld\n", pontos_dentro);
    printf("Valor estimado de Pi: %f\n", pi_estimado);
    printf("Tempo Medido: %f (s) \n", fim - inicio);

    return 0;
}