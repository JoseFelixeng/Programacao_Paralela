#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAM 60000000UL    /* tamanho do vetor (ajuste conforme a RAM disponível) */
#define REPS 5          /* repetições de cada laço para reduzir ruído na medição */

static double medirTempo(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void) {
    double *v = malloc(TAM * sizeof(double));

    // Laço 1: 
    double ti1 = medirTempo();
    for (unsigned long r = 0; r < REPS; r++) {
        for (unsigned long i = 0; i < TAM; i++) {
            v[i] = (double)i * 0.5 + 1.0;   /* cada v[i] só depende de i, não de v[i-1] */
        }
    }
    double tf1 = medirTempo();
    printf("Laco 1  %.4f s\n", (tf1 - ti1) / REPS);

    // Laço 2: soma acumulativa 
    double somaAcumulativa = 0.0;
    double ti2 = medirTempo();
    for (unsigned long r = 0; r < REPS; r++) {
        somaAcumulativa = 0.0;
        for (unsigned long i = 0; i < TAM; i++) {
            somaAcumulativa += v[i]; 
        }
    }
    double tf2 = medirTempo();
    printf("Laco 2 (soma serial)..........: %.4f s  (soma = %.3f)\n", (tf2 - ti2) / REPS, somaAcumulativa);

    //Laço 3
    double somaQuebrada = 0.0;
    double ti3 = medirTempo();
    for (unsigned long r = 0; r < REPS; r++) {
        double s0 = 0.0, s1 = 0.0, s2 = 0.0, s3 = 0.0;
        unsigned long i;
        for (i = 0; i + 4 <= TAM; i += 4) {
            s0 += v[i];
            s1 += v[i + 1];
            s2 += v[i + 2];
            s3 += v[i + 3];
        }
        for (; i < TAM; i++) {   /* trata o resto, caso TAM não seja múltiplo de 4 */
            s0 += v[i];
        }
        somaQuebrada = (s0 + s1) + (s2 + s3);
    }
    double tf3 = medirTempo();
    printf("Laco 3 (soma c/ 4 acumuladores): %.4f s  (soma = %.3f)\n", (tf3 - ti3) / REPS, somaQuebrada);

    printf("\nDiferenca somaAcumulativa - somaQuebrada = %g (esperado ~0, so erro de arredondamento)\n",
           somaAcumulativa - somaQuebrada);

    free(v);
    return 0;
}