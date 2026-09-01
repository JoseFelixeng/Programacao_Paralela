#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

int main(){
    long long N = 10000000;
    long long pontos_dentro = 0;
    long long ultimo = -1;
    double x,y;
    srand((unsigned int)time(NULL));

   double inicio = omp_get_wtime();
   #pragma omp parallel
   {
        
        long long local_dento_c = 0;
        

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