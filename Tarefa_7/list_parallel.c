#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>

#define TAM_NOME 100

typedef struct No{
    char nome[TAM_NOME];
    struct No *prox;
} No;

No *init(const char *nome){
    No *novoNo = (No *) malloc(sizeof(No));
    if(novoNo == NULL){
        printf("Erro!");
        return NULL;
    }

    strncpy(novoNo->nome, nome, TAM_NOME - 1);
    novoNo -> nome[TAM_NOME - 1] = '\0';
    novoNo -> prox = NULL;
    return novoNo;
}

void Add(No **cabeca, const char *nome){
    No *novoNo = init(nome);
    if(*cabeca == NULL){
        *cabeca = novoNo;
        return;
    }

    No *noAtual = *cabeca; 

    while(noAtual->prox != NULL){
        noAtual = noAtual->prox;
    }
     noAtual->prox = novoNo;
}

int verificar(const No *cabeca){
    int count = 0;
    const No *atual = cabeca;
    while(atual != NULL){
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
    printf("Nome: %25s | thread: %d\n", no->nome, omp_get_thread_num());
}

int main(){
    No *lista = NULL;
    const char *filmes[] = {
        "1- star_wars_uma_nova_esperanca.mp4",
        "2- o_senhor_dos_aneis_a_sociedade_do_anel.mp4",
        "3- matrix.mp4",
        "4- blade_runner_2049.mp4",
        "5- duna.mp4",
        "6- interestelar.mp4",
        "7- de_volta_para_o_futuro.mp4",
        "8- jurassic_park.mp4",
        "9- alien_o_oitavo_passageiro.mp4",
        "10- o_exterminador_do_futuro.mp4",
        "11- vingadores_ultimato.mp4",
        "12- guardioes_da_galaxia.mp4",
        "13- homem_aranha_no_aranhaverso.mp4",
        "14- jogador_numero_1.mp4",
        "15- tron_o_legado.mp4",
        "16- ghost_in_the_shell.mp4",
        "17- akira.mp4",
        "18- circulo_de_fogo.mp4",
        "19- no_limite_do_amanha.mp4",
        "20- millennium_falcon_edicao_especial.mp4"
    };

    int n_filmes = sizeof(filmes)/sizeof(filmes[0]);

    for(int i = 0; i < n_filmes; i++){
        Add(&lista, filmes[i]);
    }

    int total_nos = verificar(lista);
    printf("Total de nos na Lista: %d \n\n", total_nos);

    #pragma omp parallel 
    {
        #pragma omp single
        {
          No *atual = lista;
          
          while(atual != NULL){
            No *no = atual;
            #pragma omp task firstprivate(no)
            {
                processar(no);
            }
            atual = atual->prox;
          }
          #pragma omp taskwait
        }
        
    }

    liberar(lista);
    
    return 0;
}

