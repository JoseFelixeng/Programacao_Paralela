#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

typedef struct No {
    int num;
    struct No *prox;
}No;

No *init(const int *num){
    No *novoNo = (No *) malloc(sizeof(No));
    if(novoNo == NULL){
        printf("Erro!");
        return NULL;
    }

    novoNo->num = *num;
    novoNo -> prox = NULL;
    return novoNo;
}

void add(No **cabeca, const int *num){
    No *novoNo = init(num);
    if (*cabeca == NULL){
        *cabeca = novoNo;
        return;
    }

    No *noAtual = *cabeca;

    while(noAtual -> prox !=NULL){
        noAtual = noAtual->prox;
    }
    noAtual->prox = novoNo;
}

int verificar(const No *cabeca){
    int count = 0; 
    const No *atual = cabeca;
    while (atual != NULL){
        count++;
        atual = atual->prox;
    }
    return count;
}

void liberar(No *cabeca){
    No *noAtual = cabeca;

    while(noAtual != NULL){
        No *proximo = noAtual->prox;
        free(noAtual);
        noAtual=proximo;
    }
}

void processar(const No *no, int lista_escolhida){
    printf("Numero: %5d | lista: %d | thread: %d\n", no->num,lista_escolhida, omp_get_thread_num());
}


int main(int argc, char *argv[]){
    int N = (argc > 1) ? atoi(argv[1]): 20;
    int M = (argc > 2) ? atoi(argv[2]): 2;

    printf("Quantidade de numeros: %d \n | Numero de Listas: %d", N, M);

    No **listas = (No **) calloc(M, sizeof(No *));
    omp_lock_t *locks = (omp_lock_t *) malloc(M * sizeof(omp_lock_t));

    //Pra não confundir com i usado nas threads foi usado j;
    for(int j = 0; j < M; j++){
        omp_init_lock(&locks[j]);
    }


    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int i = 0; i < N; i++){
                #pragma omp task firstprivate(i)
                {
                    unsigned int seed = (unsigned int) time(NULL) ^ (unsigned int )(omp_get_thread_num() << 12) ^ (unsigned int) i;
                    int valor = i + 1;
                    int lista_escolhida = rand_r(&seed) % M; 

                    // Criando nó temporario 
                    No temp = {valor, NULL};
                    processar(&temp, lista_escolhida); 

                    omp_set_lock(&locks[lista_escolhida]);
                    add(&listas[lista_escolhida], &valor);
                    omp_unset_lock(&locks[lista_escolhida]);
                }
            }
        }
        #pragma omp taskwait

    }

    int total = 0;
  
    for (int k = 0 ; k < M; k++){
        int n_listas = verificar(listas[k]);
        printf("Lista %d: %d elementos \n", k, n_listas);
        total += n_listas;
    } 

    printf("Total inserido: %d (esperado: %d)\n", total, N);

    for (int k = 0 ; k < M; k++){
       liberar(listas[k]);
       omp_destroy_lock(&locks[k]);
    } 

    free(listas);
    free(locks);

    return 0;
}
