# Relatório Técnico Comparativo: Tarefas 16 a 19
**Disciplina:** DCA3703 – Programação Paralela  
**Instituição:** Departamento de Engenharia de Computação e Automação (DCA) – UFRN  
**Ambiente de Execução:** Cluster de Alto Desempenho NPAD / UFRN  

---

## Sumário Executivo e Evolução Arquitetural

As Tarefas 16 a 19 marcam a transição pedagógica e técnica entre dois dos modelos mais importantes da computação de alto desempenho contemporânea:
1. **Memória Distribuída (MPI):** Avaliação de padrões de decomposição de dados, operações coletivas de comunicação e impacto do leiaute de memória (*row-major*) em arquiteturas de clusters (Tarefas 16 e 17).
2. **Dispositivos Massivamente Paralelos / Aceleradores (OpenMP Target Offloading em GPU):** Exploração do modelo de descarregamento (*offloading*) em GPUs modernas (NVIDIA Tesla V100), analisando intensidade aritmética, concorrência massiva de *threads* e o gargalo do barramento PCI Express (Tarefas 18 e 19).

```
+---------------------------------------------------------------------------------------------------+
| MODELO DE MEMÓRIA DISTRIBUÍDA (MPI)                                                               |
|  - Tarefa 16: Multiplicação Matriz-Vetor por Linhas (Scatter + Bcast + Gather)                     |
|  - Tarefa 17: Multiplicação Matriz-Vetor por Colunas (Tipos Derivados + Reduce)                   |
+---------------------------------------------------------------------------------------------------+
                                                  |
                                                  v
+---------------------------------------------------------------------------------------------------+
| PROGRAMAÇÃO DE DISPOSITIVOS MASSIVAMENTE PARALELOS (OpenMP Target na GPU V100)                     |
|  - Tarefa 18: Adição de Vetores (vadd) - CPU vs. GPU e Baixa Intensidade Aritmética               |
|  - Tarefa 19: Equação do Calor 2D (Stencil) - Diretivas target/loop e Gargalo de Cópia PCIe       |
+---------------------------------------------------------------------------------------------------+
```

---

## 1. Tarefa 16 – Multiplicação Matriz-Vetor Distribuída com Decomposição por Linhas (MPI)

### 1.1. Enunciado e Objetivos
Implementar um programa paralelo em MPI que calcule o produto matriz-vetor $y = A \cdot x$, onde a matriz $A$ possui dimensão $M \times N$ e o vetor $x$ possui dimensão $N$. A distribuição de $A$ deve ser particionada por **linhas contíguas** utilizando `MPI_Scatter`, o vetor $x$ deve ser integralmente propagado via `MPI_Bcast`, e os segmentos resultantes de $y$ devem ser recolhidos pelo processo raiz (rank 0) através de `MPI_Gather`. O estudo exige varredura paramétrica de tamanhos de matriz e contagens de processos para análise de escalabilidade forte (*strong scaling*).

### 1.2. Modelagem Matemática e Estratégia de Comunicação
* **Decomposição por Linhas:** Como matrizes na linguagem C são dispostas em formato *row-major* (linhas contíguas na memória física), cada bloco de $local\_M = M / P$ linhas consecutivas forma um único bloco contíguo de $local\_M \times N$ elementos do tipo `double`.
* **Fluxo de Comunicação Coletiva:**
  1. `MPI_Scatter`: O processo 0 envia fatias contíguas de $local\_M \times N$ elementos de $A$ para cada um dos $P$ processos.
  2. `MPI_Bcast`: O vetor $x$ completo ($N$ elementos) é transmitido do processo 0 para todos os demais processos.
  3. **Computação Local:** Cada processo executa um laço independente calculando o produto escalar de suas $local\_M$ linhas pelo vetor $x$:
     $$local\_y[i] = \sum_{j=0}^{N-1} local\_A[i \times N + j] \cdot x[j]$$
     Nenhuma comunicação interprocessos ocorre durante a fase de cálculo (*embaraçosamente paralelo*).
  4. `MPI_Gather`: O processo 0 recebe os segmentos locais $local\_y$ ($local\_M$ elementos por processo) e os concatena ordenadamente para recompor o vetor final $y$ ($M$ elementos).

### 1.3. Metodologia Experimental
* **Plataforma:** Partição `amd-512` do cluster NPAD (1 nó dedicado com 32 núcleos, alocação `--exclusive`).
* **Compilação:** `mpicc -O3 -o multMxV multMxV.c -lm`.
* **Parâmetros:** Matrizes quadradas $M = N \in \{512, 1024, 2048, 4096, 8192\}$; processos $P \in \{1, 2, 4, 8, 16, 32\}$; cada configuração avaliada através da média de 30 repetições cronometradas (`MPI_Wtime`).
* **Verificação de Corretude:** Comparação com rotina serial no processo 0 exibindo erro absoluto máximo de $0.00\times 10^0$ em todas as configurações.

### 1.4. Resultados Obtidos (NPAD - amd-512)

| $M = N$ | $P = 1$ (s) | $P = 2$ (s) | $P = 4$ (s) | $P = 8$ (s) | $P = 16$ (s) | $P = 32$ (s) | Speedup Máx ($S_{32}$) | Eficiência ($E_{32}$) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **512**  | 0,000348 | 0,000302 | 0,000196 | 0,000145 | 0,000140 | 0,000184 | 1,89x | 5,92% |
| **1024** | 0,001357 | 0,000932 | 0,000791 | 0,000803 | 0,000343 | 0,000298 | 4,55x | 14,22% |
| **2048** | 0,006019 | 0,004066 | 0,002708 | 0,002655 | 0,000907 | 0,000818 | 7,35x | 22,98% |
| **4096** | 0,028261 | 0,018993 | 0,012481 | 0,010660 | 0,002944 | 0,002190 | 12,90x | 40,32% |
| **8192** | 0,113709 | 0,078264 | 0,059853 | 0,054659 | 0,022300 | 0,018214 | 6,24x | 19,51% |

### 1.5. Análise Crítica
1. **Complexidade Quadrática Confirmada:** Para $P = 1$, duplicar a dimensão $N$ quadruplica o tempo de execução ($\approx 4\times$), condizente com a complexidade assintótica $O(M \cdot N) = O(N^2)$.
2. **Comportamento de Escalabilidade Forte:** Conforme prescrito pela Lei de Amdahl, problemas maiores apresentam melhor aproveitamento dos recursos paralelos: para $N=512$, a sobrecarga de comunicação domina a partir de $P=16$, degradando o tempo em $P=32$; já para $N=4096$, o volume de trabalho local compensa as transferências, atingindo *speedup* de 12,90x em 32 processos.
3. **Padrão de Cache e Domínios NUMA:** Notou-se um ganho expressivo de desempenho na transição de $P=8$ para $P=16$, decorrente da alocação de múltiplos nós/domínios de memória com barramentos independentes no processador AMD.

---

## 2. Tarefa 17 – Multiplicação Matriz-Vetor Distribuída com Particionamento por Colunas (MPI)

### 2.1. Enunciado e Objetivos
Reimplementar o produto $y = A \cdot x$ distribuído via MPI substituindo a partição por linhas pela partição por **colunas**. A distribuição das colunas não contíguas exige a definição de um tipo derivado via `MPI_Type_vector` e `MPI_Type_create_resized`. Cada processo deve receber um bloco de colunas e uma fatia correspondente de $x$, calculando contribuições parciais para todos os elementos de $y$. Os vetores parciais devem ser somados no processo 0 via `MPI_Reduce` com a operação `MPI_SUM`.

### 2.2. O Desafio Estrutural dos Tipos Derivados
Na disposição *row-major*, os elementos de uma coluna estão separados por $N$ posições (`doubles`) de distância na memória:
```c
// 1. Cria padrão de M elementos com stride N
MPI_Datatype coluna_tmp, tipo_coluna;
MPI_Type_vector((int)M, 1, (int)N, MPI_DOUBLE, &coluna_tmp);

// 2. Redefine o extent para 1 sizeof(double)
// Sem redimensionar o extent, o MPI_Scatter não consegue posicionar colunas adjacentes
MPI_Type_create_resized(coluna_tmp, 0, sizeof(double), &tipo_coluna);
MPI_Type_commit(&tipo_coluna);
```
O redimensionamento do *extent* permite que o `MPI_Scatter` envie blocos de $local\_N = N/P$ colunas contíguas no índice, com deslocamento unitário.

### 2.3. Diferenças Fundamentais no Acesso à Memória (Linhas vs. Colunas)

| Critério | Decomposição por Linhas (T16) | Decomposição por Colunas (T17) |
| :--- | :--- | :--- |
| **Acesso ao vetor $y$** | **Dot-Product:** $y[i]$ é acumulado em registrador e escrito **uma única vez** na memória. | **SAXPY:** O vetor $local\_y$ inteiro ($M$ elementos) é lido e reescrito $local\_N$ vezes na memória RAM. |
| **Comunicação de $x$** | Replicação global via `MPI_Bcast` ($N$ doubles para todos). | Fatiamento contíguo via `MPI_Scatter` ($local\_N$ por processo). |
| **Coleta de $y$** | `MPI_Gather` simples (apenas movimentação de dados disjuntos). | `MPI_Reduce` com `MPI_SUM` sobre o vetor $y$ completo ($M$ elementos). |
| **Overhead de Empacotamento** | **Nenhum:** linhas são naturalmente contíguas. | **Alto:** o driver MPI precisa empacotar (*pack/unpack*) elementos esparsos. |

### 2.4. Resultados Experimentais Comparativos ($M = N = 4096$)

| Processos ($P$) | Tempo Linhas - T16 (s) | Tempo Colunas - T17 (s) | Speedup Linhas (T16) | Speedup Colunas (T17) |
| :---: | :---: | :---: | :---: | :---: |
| **1**  | 0,028261 | 0,152240 | 1,00x | 1,00x |
| **2**  | 0,018993 | 0,156544 | 1,49x | 0,97x |
| **4**  | 0,012481 | 0,152366 | 2,26x | 1,00x |
| **8**  | 0,010660 | 0,198966 | 2,65x | 0,77x *(desaceleração)* |
| **16** | 0,002944 | 0,193968 | 9,60x | 0,78x *(desaceleração)* |
| **32** | 0,002190 | 0,210760 | 12,90x | 0,72x *(desaceleração)* |

### 2.5. Conclusão da Análise Comparativa
A abordagem por colunas falhou em apresentar aceleração e escalabilidade:
* **Penalidade Serial de Cache:** Mesmo com $P=1$, a versão por colunas foi **5,4x mais lenta** que a versão por linhas. A varredura por colunas quebra o princípio da localidade espacial de cache e força leituras/escritas repetitivas sobre o vetor $y$.
* **Gargalo da Redução Global:** A rotina `MPI_Reduce` opera sobre o tamanho completo $M$, independente de $P$. À medida que o trabalho de cálculo local diminui ($M \cdot N / P$), o custo da redução e do empacotamento de memória passa a ditar o tempo total, resultando em *speedup* inferior a 1 ($S_{32} = 0,72$).
* **Diretriz Prática:** Em matrizes *row-major*, o particionamento por linhas é amplamente superior. A partição por colunas só se justifica teoricamente em linguagens com arranjo *column-major* (como Fortran/Julia) ou em matrizes já transpostas.

---

## 3. Tarefa 18 – Adição de Vetores ($vadd$): CPU Multicore vs. GPU Tesla V100 (OpenMP)

### 3.1. Enunciado e Objetivos
Adaptar o algoritmo clássico de adição de vetores ($c[i] = a[i] + b[i]$) para execução paralela em CPU multicore e acelerador GPU utilizando diretivas OpenMP (`#pragma omp target`, `#pragma omp loop`). O experimento tem como meta mensurar os tempos de execução para um vetor de grandes dimensões ($N = 10^7$ floats) em um dos nós com GPU do cluster NPAD, identificando os fatores determinantes do desempenho.

### 3.2. Implementações e Diretivas
* **Versão CPU Paralela (`vadd_p.c`):**
  ```c
  #pragma omp parallel for
  for (int i = 0; i < N; i++) { c[i] = a[i] + b[i]; }
  ```
* **Versão GPU Offloading (`vadd_gpu.c`):**
  Descarrega o laço da soma para a GPU criando um kernel computacional:
  ```c
  #pragma omp target map(to: a[0:N], b[0:N]) map(from: c[0:N])
  #pragma omp loop
  for (int i = 0; i < N; i++) { c[i] = a[i] + b[i]; }
  ```
  *(Também foi avaliada a variante com mapeamento implícito `map(tofrom: a, b, c)`).*

### 3.3. Metodologia Experimental
* **CPU:** Partição `amd-512` (AMD EPYC, gcc `-fopenmp -g -Wall`).
* **GPU:** Partição `gpu-8-v100` (NVIDIA Tesla V100-SXM2-16GB, compilador NVIDIA `nvc -mp=gpu -gpu=cc70 -O3 -Minfo=mp`).
* **Carga de Trabalho:** Vetores de ponto flutuante de precisão simples com $N = 10^7$ elementos ($\approx 40$ MB por vetor; tráfego mínimo de 120 MB).
* **Fases Mensuradas:** `Init` (preenchimento), `Compute` (soma/kernel) e `Test` (validação com redução).

### 3.4. Resultados Obtidos ($N = 10^7$)

| Arquitetura / Configuração | Máquina / Nó | Init (s) | Compute (s) | Test (s) | Total (s) | Razão vs. CPU (Compute) |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: |
| **CPU Multicore** | `amd-512` | 0,0640 | **0,0210** | 0,0290 | **0,1150** | **1,00x** |
| **GPU (Map Explícito)** | `gpu-8-v100` (V100) | 0,0554 | **0,2891** | 0,0067 | **0,3511** | **13,77x mais lenta** |
| **GPU (Map Implícito)** | `gpu-8-v100` (V100) | 0,0540 | **0,2775** | 0,0068 | **0,3384** | **13,22x mais lenta** |

### 3.5. Diagnóstico Físico do Desempenho
A execução na GPU Tesla V100 foi **cerca de 14 vezes mais lenta** no cálculo e **3 vezes mais lenta no tempo total**:
1. **Baixíssima Intensidade Aritmética:** O algoritmo realiza uma única operação de adição em troca da movimentação de três palavras de 4 bytes (leitura de $a$ e $b$, escrita de $c$).
   $$\text{Intensidade Aritmética} = \frac{1 \text{ FLOP}}{3 \times 4 \text{ bytes}} = \frac{1}{12} \approx 0,083 \text{ FLOP/B}$$
2. **Gargalo do Barramento PCIe:** A GPU não retém os dados na memória de alta velocidade (HBM2). O custo de transferir os vetores através do barramento PCIe (cujas taxas práticas ficam entre 8 e 12 GB/s) supera com folga os microssegundos necessários para os núcleos computarem a soma.
3. **Custo Fixo de Inicialização (*Target Context*):** A primeira diretiva `#pragma omp target` incorre no overhead de inicialização do runtime CUDA e do contexto do driver na GPU (tipicamente da ordem de 100 a 200 ms).

---

## 4. Tarefa 19 – Simulação da Equação do Calor 2D em GPU com OpenMP e Nsys

### 4.1. Enunciado e Objetivos
Paralelizar o código de condução de calor bidimensional por diferenças finitas (`heat.c`, stencil explícito de 5 pontos) para GPU utilizando OpenMP. O objetivo central é explorar as cláusulas de distribuição de trabalho (`#pragma omp loop`, `collapse(2)`) e as cláusulas de mapeamento de memória (`map(tofrom)`), analisando o comportamento temporal com o aumento do número de passos de tempo e perfilando os gargalos via **NVIDIA Nsys** (`nsys`).

### 4.2. Formulação e Análise Teórica do Kernel (Roofline Model)
Para uma malha discreta de dimensão $N = 8000$ ($8000 \times 8000 = 64 \times 10^6$ pontos de grade) com vetores em precisão dupla (`double`):
* **Memória da Malha:** $8000^2 \times 8\text{ B} = 512$ MB por array (dois arrays: $u$ e $u_{tmp} = 1,024$ GB).
* **Operações por Ponto:** 9 FLOPs (5 multiplicações e 4 somas).
* **Tráfego de Memória:** Mínimo de 16 Bytes por célula (leitura de $u$ e escrita de $u_{tmp}$, assumindo cache perfeita para vizinhos).
* **Intensidade Aritmética:** $\frac{9 \text{ FLOPs}}{16 \text{ Bytes}} \approx 0,56 \text{ FLOP/B}$.
* **Hardware da Tesla V100:**
  * Largura de banda de pico (HBM2): $\approx 900$ GB/s.
  * Ponto de equilíbrio de hardware: $\frac{7,8 \text{ TFLOP/s}}{900 \text{ GB/s}} \approx 8,7 \text{ FLOP/B}$.
  * **Conclusão Teórica:** Como $0,56 \ll 8,7$, a aplicação é estritamente **Memory-Bound**. Pelo modelo Roofline, o teto prático é de $0,56 \times 900\text{ GB/s} \approx 506$ GFLOP/s ($\approx 6\%$ do pico da V100). O tempo mínimo físico para processar um passo na HBM2 seria de $\approx 1,1$ ms.

### 4.3. Implementações Avaliadas
As diretivas foram testadas dentro da função `solve()`:
* **T1 / E0 (CPU Serial):** Baseline sequencial em C (`gcc -g -fopenmp`).
* **T2 / E1 (GPU Target Puro):**
  ```c
  #pragma omp target map(tofrom: u[0:n*n], u_tmp[0:n*n])
  for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) { /* Stencil de 5 pontos */ }
  ```
* **T3 / E2 (GPU Target + Loop):** Adiciona `#pragma omp loop` sobre o primeiro laço `for`.
* **T4 / E3 / E4 (GPU Target + Loop + Collapse):**
  ```c
  #pragma omp target map(tofrom: u[0:n*n], u_tmp[0:n*n])
  #pragma omp loop collapse(2)
  for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) { /* Stencil de 5 pontos */ }
  ```

### 4.4. Resultados Experimentais (NPAD - V100, $N = 8000$)

| Teste | Estratégia de Código | Passos de Tempo | Tempo do Solve (s) | Tempo Total (s) | Tempo / Passo (s) | Observações / Log |
| :---: | :--- | :---: | :---: | :---: | :---: | :--- |
| **T1** | CPU Serial (gcc -g) | 10 | 5,995 | 10,029 | 0,600 | Baseline sequencial |
| **T2** | GPU: target (sem loop) | 10 | 139,854 | 141,828 | 13,985 | Executou em **1 thread** na GPU |
| **T3** | GPU: target + loop | 10 | 2,413 | 4,466 | 0,241 | Laço externo paralelizado em teams(128) |
| **T4** | GPU: target + loop collapse(2) | 10 | 2,455 | 4,539 | 0,246 | Laços colapsados ($6,4 \times 10^7$ iterações) |
| **T7** | GPU: loop collapse(2) (nsys) | 500 | 107,396 | 109,390 | 0,215 | Escalonamento linear com passos |
| **T8** | GPU: loop collapse(2) (nsys) | 1000 | 210,495 | 212,482 | 0,210 | Cópia PCIe domina 96% do tempo |

### 4.5. Diagnóstico Aprofundado com NVIDIA Nsys
O perfilamento com o Nsys no teste T8 (1000 passos) revelou com precisão a anatomia do tempo de execução:
* **Tempo de Cálculo do Kernel na GPU:** Apenas **7,49 ms por passo** (com o kernel devidamente paralelizado em blocos e threads).
* **Tempo de Cópia PCIe:** Cerca de **202 segundos** dos 210,5 segundos totais foram gastos em operações `cudaMemcpy` (ida e volta dos arrays de 512 MB a cada passo).
* **Volume de Dados Trafegado:** Mais de **2,05 TB** transferidos pelo barramento PCIe durante a simulação.
* **A Falha de Estrutura:** Como o bloco `#pragma omp target map(tofrom: ...)` estava encapsulado dentro da função `solve()`, o ambiente de dados da GPU era alocado, copiado e destruído **a cada iteração temporal**.

---

## 5. Síntese Comparativa das Tarefas 16 a 19

| Dimensão | Tarefa 16 | Tarefa 17 | Tarefa 18 | Tarefa 19 |
| :--- | :--- | :--- | :--- | :--- |
| **Modelo** | Memória Distribuída (MPI) | Memória Distribuída (MPI) | Acelerador Heterogêneo (OpenMP) | Acelerador Heterogêneo (OpenMP) |
| **Problema** | Multiplicação $y = A \cdot x$ | Multiplicação $y = A \cdot x$ | Adição de Vetores ($vadd$) | Equação do Calor 2D (Stencil) |
| **Divisão de Carga** | Linhas contíguas de $A$ | Colunas esparsas de $A$ | Fatias 1D de vetores | Grade 2D ($collapse(2)$) |
| **Primitivas Centrais** | `MPI_Scatter`, `MPI_Bcast`, `MPI_Gather` | `MPI_Type_vector`, `MPI_Type_create_resized`, `MPI_Reduce` | `#pragma omp target map`, `#pragma omp loop` | `#pragma omp target map`, `loop collapse(2)` |
| **Hardware** | 32 núcleos CPU (`amd-512`) | 32 núcleos CPU (`amd-512`) | Tesla V100 vs. CPU AMD | Tesla V100-SXM2-16GB |
| **Desempenho** | **Excelente** ($S_{32} = 12,9\times$) | **Fraco / Degradação** ($S_{32} = 0,72\times$) | **Desfavorável à GPU** (14x mais lenta no compute) | **Limitado por Cópia** ($\approx 0,21$ s/passo) |
| **Causa Primária do Gargalo** | Sobrecarga de comunicação fixa em matrizes pequenas (Amdahl). | Quebra de localidade em *row-major*, reescritas no vetor $y$ e custo do `Reduce`. | Intensidade aritmética nula ($0,08$ FLOP/B) + latência de inicialização CUDA. | Transferência redundante de 1 GB via PCIe a cada passo de tempo ($map$ dentro do laço). |

---

## 6. Projeções e Conexão Direta com a Tarefa 20

A análise da Tarefa 19 pavimenta o caminho exato para a **Tarefa 20**:
* Na Tarefa 19, o tempo por passo foi de $\approx 210$ ms (dos quais $\approx 202$ ms foram cópias PCIe e apenas $7,5$ ms foram kernel).
* **Solução da Tarefa 20:** Deslocar o gerenciamento de dados para fora do laço temporal através da diretiva estruturada:
  ```c
  #pragma omp target data map(tofrom: u[0:n*n]) map(alloc: u_tmp[0:n*n])
  {
      for (int step = 0; step < num_steps; ++step) {
          solve(n, alpha, dx, dt, u, u_tmp);
      }
  }
  ```
* **Impacto Projetado:** Com os dados residentes na memória HBM2 da GPU durante todos os 1000 passos, o tempo de execução cai de **210,5 segundos para aproximadamente 7,6 segundos**, proporcionando um **speedup de 27,7x** sobre a Tarefa 19 e liberando o potencial da arquitetura massivamente paralela.
