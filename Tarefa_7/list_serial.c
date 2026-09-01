#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <time.h>

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

void print(const No *no){
    printf("Nome: %25s | thread: %d\n", no->nome, omp_get_thread_num());
}

int main(){
    No *lista = NULL;
    const char *filmes[] = {
        "star_wars_uma_nova_esperanca.mp4",
        "o_senhor_dos_aneis_a_sociedade_do_anel.mp4",
        "matrix.mp4",
        "blade_runner_2049.mp4",
        "duna.mp4",
        "interestelar.mp4",
        "de_volta_para_o_futuro.mp4",
        "jurassic_park.mp4",
        "alien_o_oitavo_passageiro.mp4",
        "o_exterminador_do_futuro.mp4",
        "vingadores_ultimato.mp4",
        "guardioes_da_galaxia.mp4",
        "homem_aranha_no_aranhaverso.mp4",
        "jogador_numero_1.mp4",
        "tron_o_legado.mp4",
        "ghost_in_the_shell.mp4",
        "akira.mp4",
        "circulo_de_fogo.mp4",
        "no_limite_do_amanha.mp4",
        "millennium_falcon_edicao_especial.mp4"
    };

    int n_filmes = sizeof(filmes)/sizeof(filmes[0]);

    for(int i = 0; i < n_filmes; i++){
        Add(&lista, filmes[i]);
    }

    int total_nos = verificar(lista);
    printf("Total de nos na Lista: %d \n\n", total_nos);

    No *atual = lista;
    while(atual != NULL){
        print(atual);
        atual = atual->prox;
    }

    liberar(lista);
    return 0;
}

