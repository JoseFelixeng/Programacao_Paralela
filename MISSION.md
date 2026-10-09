# Mission: DCA3703 (Programação Paralela, UFRN)

## Why

Implementar, perfilar e defender as 13 tarefas práticas em C e OpenMP da disciplina DCA3703 na UFRN.

## Success looks like

- Entregar o código C e o relatório em LaTeX para cada uma das 13 tarefas:
  - **Tarefa 01 (Série de Leibniz)**: Ponto flutuante `double`, erro de truncamento e medição do custo de convergência.
  - **Tarefa 02 (MxV Row vs Col-major)**: Acesso contíguo à memória, linhas de cache de 64 bytes e impacto de cache misses.
  - **Tarefa 03 (ILP)**: Quebra de dependências RAW via múltiplos acumuladores e teste com `-O0`, `-O2` e `-O3`.
  - **Tarefa 04 (Memory vs Compute bound)**: Comparação de laços limitados por barramento vs CPU com OpenMP e SMT.
  - **Tarefa 05 (Contagem de primos)**: Paralelização com OpenMP e diagnóstico de desbalanceamento de carga.
  - **Tarefa 06 (Monte Carlo e escopos)**: Correção de condição de corrida com `critical` e escopos `private`, `firstprivate`, `shared` e `default(none)`.
  - **Tarefa 07 (Tarefas em lista)**: Paralelismo com `#pragma omp task`, `taskwait` e `single`.
  - **Tarefa 08 (Coerência e false sharing)**: Comparação de `rand` vs `rand_r` e eliminação de contenção de linha de cache.
  - **Tarefa 09 (Sincronização em listas)**: Uso de `omp_lock_t` para proteger inserções em $N$ listas encadeadas.
  - **Tarefa 10 (Comparativo de reduções)**: Medição de overhead entre `atomic`, `critical`, contadores privados e `reduction`.
  - **Tarefa 11 (difusão viscosa 2D, simplificação de Navier–Stokes)**: Implementação de difusão com diferenças finitas em OpenMP usando `collapse` e `schedule`.
  - **Tarefa 12 (Escalabilidade no NPAD)**: Medição de escalabilidade forte e fraca no cluster NPAD.
  - **Tarefa 13 (Afinidade de threads)**: Ancoragem de threads com `OMP_PROC_BIND` e `OMP_PLACES` em nós NUMA.
- Responder a perguntas técnicas de código e arquitetura durante as defesas orais.

## Constraints

- Ambiente: Linux x86_64, GCC, OpenMP e cluster NPAD/UFRN.
- Código: C99 ou C11 com temporização via `clock_gettime(CLOCK_MONOTONIC)`.

## Out of scope

- Linguagens fora do escopo da disciplina (Python apenas para geração de gráficos).
- Modelos MPI ou CUDA não solicitados no roteiro da disciplina.
