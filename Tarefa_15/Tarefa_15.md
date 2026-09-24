# Tarefa 15 — Programação em memória distribuída (difusão de calor 1D com MPI)

**DCA3703 · Programação Paralela · UFRN** · Prof. Samuel Xavier de Souza · Aluno: José Felix Rodrigues Anselmo

## Objetivo

Simular a difusão de calor em uma barra 1D dividida entre dois ou mais processos MPI e comparar **três estratégias de troca de bordas** (halo exchange):

1. Bloqueante: `MPI_Send` / `MPI_Recv`.
2. Não bloqueante com espera imediata: `MPI_Isend` / `MPI_Irecv` + `MPI_Wait`.
3. Não bloqueante com sobreposição: `MPI_Isend` / `MPI_Irecv` + `MPI_Test`, calculando o interior enquanto a comunicação ocorre.

## Conceitos-chave

- **Células fantasma (ghost cells):** cada processo guarda `m + 2` posições: `m` pontos reais (índices 1 a m) e 2 fantasmas (índices 0 e m+1), que recebem as bordas dos vizinhos a cada passo.
- **Esquema FTCS:** `u_i^(n+1) = u_i^n + c·(u_(i-1)^n − 2u_i^n + u_(i+1)^n)`, com c = 0,25.
- **Pontos internos** (2 ≤ i ≤ m−1) não dependem das fantasmas e podem ser calculados a qualquer momento. **Pontos de borda** (i = 1 e i = m) só depois da recepção.
- **Contorno global:** o vizinho é `MPI_PROC_NULL` e a fantasma fica em zero (Dirichlet nulo).
- **Anti-deadlock no modo 1:** ranks pares enviam primeiro e recebem depois. Ranks ímpares recebem primeiro e enviam depois.
- **Condição inicial:** pulso de amplitude 100 no terço central da barra. Divisão uniforme dos pontos entre os processos, com o resto repartido entre os primeiros.

## Ordem das operações por passo

| Versão | Passo 1 | Passo 2 | Passo 3 |
| --- | --- | --- | --- |
| Send/Recv | Trocar bordas (ordem por paridade) | Calcular pontos 1..m | Trocar ponteiros |
| Isend/Irecv + Wait | Iniciar 4 operações | Wait nas 4, depois calcular 1..m | Trocar ponteiros |
| Isend/Irecv + Test | Iniciar 4 operações | Calcular interior [2..m−1] em blocos de 2000 com `MPI_Testall` a cada bloco, `MPI_Waitall` se pendente, depois bordas 1 e m | Trocar ponteiros |

## Como compilar e executar

```bash
mpicc -O3 -march=native -o barra barra1d.c -lm
mpirun -np <P> ./barra <modo> <N> <passos>
# modo: 1 = Send/Recv | 2 = Isend/Irecv + Wait | 3 = Isend/Irecv + Test
# exemplo: mpirun -np 2 ./barra 3 1000000 1000
```

Scripts SLURM (partição `amd-512`):

- **Job 2118946:** 2 nós, 2 tarefas, compilação sem flags de otimização, três modos em sequência.
- **Job 2121424 (escalonamento):** 2 nós, 32 tarefas, `-O3 -march=native`, para P = 2, 4, 8, 16, 32, três modos e 3 repetições cada (**45 medições**).

Parâmetros: N = 1 000 000 células, 1000 passos. O tempo medido inclui só o laço de passos e a barreira final.

## Resultados

**Job inicial (P = 2, sem otimização, uma execução por versão):**

| Versão | Tempo (s) |
| --- | --- |
| 1 — Send/Recv | 1,884325 |
| 2 — Isend/Irecv + Wait | 1,900909 |
| 3 — Isend/Irecv + Test | 1,821498 |

Uma execução isolada anterior do modo 3 (job 2118944) mediu 1,677861 s, cerca de 8% menos que os 1,821498 s acima. Isso mostra a variabilidade entre submissões em um cluster compartilhado.

**Experimento de escalonamento** (mediana de 3 repetições, em segundos, `-O3 -march=native`):

| P | Modo 1: Send/Recv | Modo 2: Wait | Modo 3: Test |
| --- | --- | --- | --- |
| 2 | **0,562369** | 0,692808 | 0,573124 |
| 4 | 0,310695 | 0,356481 | **0,275765** |
| 8 | 0,149582 | 0,170687 | **0,137490** |
| 16 | 0,083914 | 0,094285 | **0,082841** |
| 32 | 0,071243 | 0,085743 | **0,067477** |

A soma global foi **33 333 400,000000** em todas as 45 execuções, o que confirma consistência numérica entre modos e valores de P.

## Conclusões

- O modo 3 teve a menor mediana em P = 4, 8, 16 e 32, com vantagens de 1,3% a 11,2% sobre o modo 1. Em P = 2, o modo 1 foi 1,9% mais rápido.
- O modo 2 foi o mais lento em todos os valores de P: espera a comunicação antes de calcular, sem sobreposição.
- A vantagem do modo 3 **não cresce de forma monotônica** com P.
- Todos os modos reduzem o tempo de P = 2 até P = 32, mas o ganho incremental diminui.

## Limitações

- Alta dispersão em P = 16 e P = 32. No modo 3 com P = 16, a amplitude entre repetições foi de 50,8% da mediana. Diferenças pequenas entre medianas pedem cautela.
- A ordem das execuções foi fixa (agrupada por P e por modo) e a distribuição dos ranks por nó não foi registrada.
- A diferença entre o job sem otimização e o otimizado não isola o efeito das flags, pois são submissões distintas.
- O kernel é compatível com acesso intensivo à memória, mas não foram medidas taxas de cache nem largura de banda.

## Perguntas para revisão

1. Por que a ordem par/ímpar de `Send`/`Recv` evita deadlock em uma cadeia 1D?
2. Por que o modo 2 não ganha nada sobre o bloqueante, apesar de usar chamadas não bloqueantes?
3. Como o modo 3 sobrepõe computação e comunicação, e qual o papel dos pontos internos?
4. Que efeito o tamanho do bloco (2000 pontos) e o custo do `MPI_Testall` podem ter?
5. Por que, com P grande, há menos trabalho interno por processo para esconder a comunicação?
