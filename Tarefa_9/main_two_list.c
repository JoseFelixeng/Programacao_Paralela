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
    No *lista1 = NULL;
    No *lista2 = NULL;

    printf("Quantidade de numeros: %d \n", N);

    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int i = 0; i < N; i++){
                #pragma omp task firstprivate(i)
                {
                    unsigned int seed = (unsigned int) time(NULL) ^ (unsigned int )(omp_get_thread_num() << 12) ^ (unsigned int) i;
                    int valor = i + 1;
                    int lista_escolhida = rand_r(&seed) % 2; // 0 ou 1 

                    // Criando nó temporario 
                    No temp = {valor, NULL};
                    processar(&temp, lista_escolhida); 

                    if (lista_escolhida == 0){
                        #pragma omp critical(lock_lista1)
                        {
                            add(&lista1, &valor);
                        }
                    }else{
                        #pragma omp critical(lock_lista2)
                        {
                            add(&lista2, &valor);
                        }
                    }


                }
                #pragma omp taskwait
            }
        }

    }

    int total1 = verificar(lista1);
    int total2 = verificar(lista2);

    printf("Lista 1: %d elementos\n", total1);
    printf("Lista 2: %d elementos\n", total2);
    printf("Total inserido: %d (esperado: %d)\n", total1+ total2, N);

    liberar(lista1);
    liberar(lista2);

    return 0;
}
