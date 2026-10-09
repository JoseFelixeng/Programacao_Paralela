#include <stdio.h>
#include <math.h>
#include <omp.h>

#define N 1000
#define M 1000

int main() {
    double matriz[N][M];
    double vetorA[N*M], vetorB[N*M], vetorC[N*M];
    
    // Inicializando vetores para o exemplo SIMD
    for(int i = 0; i < N*M; i++) {
        vetorA[i] = i * 1.5;
        vetorB[i] = i * 2.5;
    }

    // 1. COLLAPSE e SCHEDULE STATIC
    // 'collapse(2)' transforma os dois loops "for" em um único loop de N*M iterações.
    // 'schedule(static, 50)' divide este loopão em blocos (chunks) de 50 iterações 
    // e os distribui de forma fixa e previsível entre as threads.
    #pragma omp parallel for collapse(2) schedule(static, 50)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            // Carga de trabalho constante: ideal para 'static'
            matriz[i][j] = i + j; 
        }
    }

    // 2. SCHEDULE DYNAMIC
    // 'dynamic' é ideal para quando o tempo de execução de cada iteração varia.
    // As threads pegam blocos de 10 em 10 (chunksize). Quando uma thread termina,
    // ela pede dinamicamente o próximo bloco.
    #pragma omp parallel for schedule(dynamic, 10)
    for (int i = 0; i < N; i++) {
        // Simulando carga desbalanceada: linhas ímpares dão muito mais trabalho
        int trabalho = (i % 2 == 0) ? 100 : 10000; 
        
        for (int w = 0; w < trabalho; w++) {
            matriz[i][0] += sin(w) * cos(w);
        }
    }

    // 3. SCHEDULE GUIDED
    // 'guided' é similar ao 'dynamic', mas o tamanho do chunk não é fixo.
    // Ele começa com blocos grandes e vai diminuindo exponencialmente até 
    // atingir o tamanho mínimo definido no chunksize (neste caso, 5).
    // Ideal para balanceamento de carga com menos sobrecarga (overhead) que o dynamic.
    #pragma omp parallel for schedule(guided, 5)
    for (int i = 0; i < N; i++) {
        // Carga decrescente: as primeiras iterações são mais pesadas
        int trabalho = (N - i) * 10; 
        
        for (int w = 0; w < trabalho; w++) {
            matriz[i][1] += sqrt(w * 2.0);
        }
    }

    // 4. PRAGMA OMP SIMD
    // Não cria novas threads! Ele instrui o compilador a usar registradores 
    // vetoriais da CPU (como AVX ou SSE) para calcular múltiplos valores em um único ciclo de clock.
    #pragma omp simd
    for (int i = 0; i < N * M; i++) {
        // Operações independentes, elemento a elemento, sem loops complexos internos.
        vetorC[i] = vetorA[i] + vetorB[i] * 3.1415;
    }

    printf("Processamento concluído com sucesso.\n");
    return 0;
}