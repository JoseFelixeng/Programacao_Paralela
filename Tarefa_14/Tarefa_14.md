# Tarefa 14 — Programação em memória distribuída (MPI ping-pong)

**DCA3703 · Programação Paralela · UFRN** · Prof. Samuel Xavier de Souza · Aluno: José Felix Rodrigues Anselmo

## Objetivo

Implementar um ping-pong entre exatamente dois processos MPI: o rank 0 envia, o rank 1 responde com o mesmo conteúdo. Medir com `MPI_Wtime` o tempo de várias trocas para mensagens de 8 bytes até pelo menos 1 MB, e identificar onde a **latência** domina e onde a **transferência de bytes** domina.

## Conceitos-chave

- Cada processo MPI tem seu **próprio espaço de endereçamento**. Toda troca de dados é explícita.
- Rotinas usadas: `MPI_Init`, `MPI_Finalize`, `MPI_Comm_rank`, `MPI_Comm_size`, `MPI_Send`, `MPI_Recv`, `MPI_Barrier`, `MPI_Wtime`.
- **Ordem que evita deadlock:** o rank 0 envia e depois recebe. O rank 1 recebe e depois envia.
- **Modelo de Hockney:** `T(n) = α + n/β`
  - α: latência fixa (mensagem de tamanho tendendo a zero);
  - β: largura de banda assintótica;
  - **n\* = α·β:** tamanho de cruzamento. Abaixo dele domina a latência, acima domina a banda.

## Método

- Mensagens de 8 bytes a 8 MB, em potências de 2.
- Um único buffer reaproveitado entre os tamanhos.
- `MPI_Barrier` antes de cada medição.
- O rank 0 mede **1000 pares** de ida e volta. O tempo por mensagem é `tempo_total / (2 × 1000)`, uma estimativa do tempo em um sentido, supondo ida e volta simétricas.
- Uma execução por tamanho, sem repetições independentes do experimento completo e sem aquecimento.

## Como executar (NPAD, partição `amd-512`)

```bash
mpicc -o main main.c
mpirun main
```

| Configuração | Trecho do script SLURM | Meio de comunicação |
| --- | --- | --- |
| 1 nó | `--nodes=1 --ntasks=2 --cpus-per-task=1` | Memória compartilhada |
| 2 nós | `--nodes=2 --ntasks=2 --cpus-per-task=1` | Rede de interconexão |

Demais opções em ambos: `--time=0-0:10`, `--partition=amd-512`, `--output=%x_%j.out`, `--error=%x_%j.err`.

## Resultados

Parâmetros do modelo de Hockney ajustados:

| Configuração | Latência (α) | Banda assintótica (β) | Cruzamento (n\*) |
| --- | --- | --- | --- |
| 1 nó (memória compartilhada) | ≈ 0,12 µs | ≈ 20,5 GB/s | ≈ 2,41 KiB |
| 2 nós (rede) | ≈ 1,80 µs | ≈ 11,7 GB/s | ≈ 20,64 KiB |

Alguns pontos medidos (tempo por mensagem):

| Tamanho | 1 nó | 2 nós |
| --- | --- | --- |
| 8 B | 4,18 µs (outlier) | 1,64 µs |
| 1 KiB | 0,36 µs | 3,04 µs |
| 64 KiB | 3,89 µs | 22,5 µs |
| 1 MiB | 54,5 µs | 94,7 µs |
| 8 MiB | 756,5 µs | 698,0 µs |

> As colunas de "banda efetiva" das tabelas do relatório batem com `bytes / tempo / 2^20`, ou seja, são MiB/s, mesmo rotuladas como MB/s.

## Conclusões

- Entre 8 e 64 bytes, o tempo varia pouco: **domina a latência**. A partir de algumas dezenas de KB, o tempo cresce quase linearmente com o tamanho: **domina a banda**.
- A rede tem latência ~15× maior (1,80 µs vs. 0,12 µs) e banda menor (11,7 vs. 20,5 GB/s) que a memória compartilhada.
- Quanto maior a latência do meio, maior o n\*: 2,41 KiB em um nó e 20,64 KiB entre nós. Aplicações com muitas mensagens pequenas entre nós sofrem mais.
- **Ruído:** o ponto de 8 B em 1 nó (4,18 µs) destoa da tendência, atribuído à falta de aquecimento (custos de inicialização do MPI e paginação na primeira chamada). Os pontos de 128 B e 512 KiB também se desviam, compatíveis com jitter do sistema.
- Melhorias futuras: iterações de aquecimento e repetições independentes do experimento.

## Perguntas para revisão

1. O que α e β representam no modelo de Hockney e como se obtém n\*?
2. Por que o rank 0 e o rank 1 chamam `Send`/`Recv` em ordens opostas?
3. Por que dividir o tempo por `2 × 1000`, e qual a hipótese por trás disso?
4. Por que o n\* é maior entre nós do que dentro de um nó?
5. Por que o ponto de 8 bytes em 1 nó é um outlier, e como um aquecimento ajudaria?
