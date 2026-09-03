#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

int main(){
    long long N = 10000000;
    long long pontos_dentro = 0;
    long long ultimo = -1;
    double x = 0.0,y = 0;

    srand((unsigned int)time(NULL));

    int num_threads = omp_get_max_threads();
    long long *acertos = calloc(num_threads, sizeof(long long));

   double inicio = omp_get_wtime();
   #pragma omp parallel default(none) shared(N, pontos_dentro, ultimo, acertos, num_threads) firstprivate(x,y)
   {
        long long local_dento_c = 0;
        int tid = omp_get_thread_num();

        #pragma omp for lastprivate(ultimo)  
        for(long long i = 0; i < N; i++){
        //Gear as coordenadas de x e y entre 0 e 1
            x = (double)rand()/RAND_MAX;
            y = (double)rand()/RAND_MAX;

            if ((x * x) + (y * y) <= 1.0)
            {
                local_dento_c++;
            }
            ultimo = i;
        }
        acertos[tid] = local_dento_c;
   }

    double fim = omp_get_wtime();

    for(int i = 0; i < num_threads; i++){
        pontos_dentro += acertos[i];
    } 

    double pi_estimado = 4.0 * ((double)pontos_dentro / N);

    printf("Pontos sorteados: %lld\n", N);
    printf("Pontos dentro do círculo: %lld\n", pontos_dentro);
    printf("Valor estimado de Pi: %f\n", pi_estimado);
    printf("Tempo Medido: %f (s) \n", fim - inicio);

    free(acertos);
    return 0;
}