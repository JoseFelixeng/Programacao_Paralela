#include <stdio.h>
#include <stdlib.h>
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

void processar(const No *no){
    printf("Numero: %25d | thread: %d\n", no->num, omp_get_thread_num());
}


int main(){
    int N = 4;
    No *lista1 = NULL;
    No *lista2 = NULL;

    const int nums[] = {1, 2, 3, 4};

    int n_num = sizeof(nums)/sizeof(nums[0]);

    printf("Quantidade de numeros: %d \n", n_num);

    #pragma omp parallel num_threads(2)
    {
        int tid = omp_get_thread_num();
        No **n_listas;
        if (tid == 0){
            n_listas = &lista1;
        }else{
            n_listas = &lista2;
        }

        for (int i =0; i < N; i++){
            #pragma omp task firstprivate(i)
            {
                No temp = {nums[i], NULL};
                processar(&temp);
    
                #pragma omp critical
                {
                    add(n_listas, &nums[i]);
                }
            }
        }
        #pragma omp taskwait

    }

    printf("Lista 1: %d elementos \n", verificar(lista1));
    printf("Lista 2: %d elementos \n", verificar(lista2));

    liberar(lista1);
    liberar(lista2);

    return 0;
}
