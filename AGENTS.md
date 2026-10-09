# Diretrizes para agentes (DCA3703)


## 1. Regra para o fluxo de ensino (/teach)

Para cada tarefa da disciplina (Tarefas 01 a 13), o agente deve gerar **um único documento HTML** em `./lessons/`, reunindo teoria e prática.

### Documento único da tarefa (ex: `lessons/0001-tarefa01-leibniz-pi.html`)

O documento deve cobrir, sem repetir conteúdo:

1. **Fundamentos de hardware**: Explicação física dos gargalos da tarefa (Memory Wall, Power Wall, gargalo de von Neumann, hierarquia de cache, linhas de 64 bytes, write-through vs write-back, dirty/invalid bits, snooping vs diretório, dependências RAW, falso compartilhamento, NUMA). Usar `assets/index.md` (slides do professor) como fonte primária do vocabulário e dos objetivos do tópico.
2. **Modelagem matemática**: Fórmulas de convergência, erro de truncamento, representação numérica (IEEE 754), speedup, leis de Amdahl e Gustafson.
3. **Contexto em HPC**: Motivação do problema e limites de hardware testados.
4. **Apêndice de referência rápida** (seção final obrigatória no mesmo arquivo):
   - **Tabelas de sintaxe**: Tipos C, funções da biblioteca padrão, pragmas e flags do compilador.
   - **Resumo executivo**: Fórmulas essenciais, constantes e regras práticas.
   - Deve usar `<section id="referencia-rapida">` com `<h2>Apêndice: Referência rápida</h2>` para âncora direta.

5. **Prática**: análise linha a linha do código C e medição do que foi executado de verdade (flags, threads, N e interpretação das curvas).
6. **Quizzes simétricos**: criar ao menos quatro questões de múltipla escolha quando o conteúdo permitir, com opções de tamanho e contagem de palavras idênticos.
7. **Navegação**: usar links relativos para o índice `index.html`, as lições em `lessons/`, código e relatório.

Concentrar teoria, prática e referência rápida no documento único de cada tarefa em `lessons/`. Não recriar o diretório obsoleto `reference/` nem seus redirecionamentos.

## 2. Portabilidade, GitHub Pages e links

- **Caminhos relativos internos**: Todo link dentro dos arquivos HTML deve usar caminhos relativos (ex: `../assets/style.css`, `../lessons/0001-tarefa01-leibniz-pi.html#referencia-rapida`, `../tarefa01/tarefa01.c`).
- **Links no README.md**:
  - Materiais HTML (lições, teoria e referência rápida) devem usar URLs absolutas do GitHub Pages: `https://eugeniovlopes.github.io/programacao-paralela/...` para abrir a página renderizada diretamente no navegador.
  - Códigos C e relatórios PDF devem usar links relativos do repositório (`tarefaXX/` e `tarefaXX/relatorio.pdf`).
- **Atualização contínua de índices**: Ao concluir qualquer tarefa, atualizar obrigatoriamente:
  1. `index.html` na raiz (adicionar o card da tarefa com links para o material único — incluindo âncora `#referencia-rapida` se preciso — e relatório).
  2. `README.md` (adicionar a linha na tabela de tarefas com links para o GitHub Pages).

## 3. Padrões de Git e arquivos ignorados

- **Commits atômicos por tarefa**: Estruturar mensagens no padrão Conventional Commits (`feat(tarefaXX): ...`, `docs: ...`, `chore: ...`).
- **Arquivos não versionados**:
  - Roteiros de apresentação e podcast (`*roteiro*.md`, `*podcast*.md`) não devem ser commitados.
  - Binários compilados e artefatos de build LaTeX (`.aux`, `.log`, `.out`, `.fls`, `.fdb_latexmk`) devem permanecer ignorados via `.gitignore`.

## 4. Padrões de código e compilação

- **Linguagem**: C (padrão C99 ou C11) em ambiente Linux x86_64.
- **Compilador**: GCC ou Clang com flags explícitas (`-O0`, `-O2`, `-O3`, `-fopenmp`, `-lm`, `-Wall`).
- **Medição de tempo**: Usar `clock_gettime(CLOCK_MONOTONIC)` com `#define _POSIX_C_SOURCE 199309L`. Na Tarefa 11, usar `omp_get_wtime()` diretamente, conforme preferência do usuário; compilar todas as versões com `-fopenmp`, mantendo v0 sem região paralela.
- **Ambiente de execução**: Nós de computação do NPAD/UFRN (suporte a NUMA e políticas `OMP_PROC_BIND` e `OMP_PLACES`).

## 5. Padrões visuais e HTML

- **Estilos**: Usar a folha de estilo compartilhada [assets/style.css](assets/style.css).
- **Matemática**: Renderização com KaTeX configurado para reconhecer delimitadores `$ ... $` e `$$ ... $$` via `applyMath()`.
- **Caracteres especiais**: Escapar operadores relacionais em blocos `<pre><code>` (`&lt;` e `&gt;`).

## 6. Relatórios técnicos

O título da Tarefa 11 é **Particionamento de dados e balanceamento de carga**.

Os relatórios devem ser documentos técnicos. Priorizar a explicação dos conceitos de programação paralela e sua relação com o código.

- Implementar o escopo pedido no enunciado e manter artefatos complementares em `tarefaXX/extras/`.
- Manter o código apresentado no relatório o mais enxuto possível. Preferir funções da biblioteca a wrappers desnecessários, como `omp_get_wtime()` em vez de `agora()` na Tarefa 11, e evitar conversores genéricos como `integer()` quando o problema não exige esse recurso. Na Tarefa 11, escrever o chunk 16 diretamente nos pragmas, sem macro `CHUNK_SIZE` nem blocos de proteção `_OPENMP`; manter `-fopenmp` nos comandos de compilação.
- Não criar modos por argumentos de linha de comando apenas para selecionar condições iniciais. Preferir a inicialização diretamente no código, alterada para cada caso de validação. Na Tarefa 11, manter os modos somente na v0 para validação; v1 a v6 devem fixar a gaussiana e aceitar apenas o arquivo de saída opcional. A simplificação da interface não dispensa verificar os campos e a perturbação exigidos pelo enunciado, nem tratar falhas de alocação ou de escrita quando essas operações existirem.
- Resumir a física e a matemática do problema ao necessário para entender o cálculo, as dependências e a paralelização.
- Manter a tabela de resultados e o gráfico de desempenho como referência. Simplificar o texto não significa remover esses elementos.
- Evitar análise detalhada dos números no texto, como enumerar tempos por versão e quantidade de threads ou repetir valores da tabela.
- Relacionar tendências observadas aos conceitos envolvidos: distribuição de trabalho, `schedule`, `collapse`, granularidade dos chunks, sincronização, localidade de memória, cache e custos do OpenMP.
- Distinguir tendências medidas de hipóteses sobre suas causas. Não atribuir uma curva a falso compartilhamento, saturação de memória ou vetorização sem evidência específica.
- Não mencionar arquivos ou comandos `.sh` no texto nem nos exemplos do relatório. Descrever a validação e a medição diretamente.
- Organizar a explicação por comparação direta entre versões. Mostrar o trecho ou pragma alterado, o que muda no trabalho de cada thread e por que isso pode afetar o desempenho. Usar exemplos pequenos de distribuição ou de ordem de execução; evitar definições genéricas sem ligação com o código. Reunir em uma seção comum os mecanismos que não mudam entre versões.
- Ao explicar versões sucessivas, referenciar somente a linha, pragma ou trecho que mudou. Descrever o código comum uma vez e deixar os arquivos completos no apêndice; não repetir o programa inteiro em cada versão.
- Quando o relatório tiver várias versões do programa, usar uma sequência estável: enunciado, fundamentação, experimento, resultados e conclusão. Descrever primeiro o que todas as versões mantêm em comum e depois comparar apenas a alteração de cada versão.
- Na seção de experimento, registrar malha, passos, ambiente, compilador, flags, número de threads, repetições e o que entra no intervalo cronometrado. Separar configuração do experimento da interpretação dos resultados.
- Na seção de resultados, separar desempenho, speedup, eficiência, escalabilidade forte e escalabilidade fraca quando houver dados para cada indicador. Manter tabelas e gráficos como referência e comentar tendências relacionando-as a um conceito específico, como overhead de criação, distribuição de trabalho, barreiras, localidade ou acesso à memória.
- Declarar quando um indicador não foi medido. Não inferir escalabilidade fraca a partir de um experimento de tamanho fixo; não atribuir uma causa única a uma curva sem medição que a sustente.
- Encerrar com uma conclusão curta que retome a melhor configuração observada, o principal gargalo e o limite da evidência. Deixar os códigos completos em um apêndice separado da discussão.
- Usar frases claras e diretas, sem repetições ou conclusões genéricas. Ao alterar o relatório LaTeX, recompilar e conferir o PDF correspondente.
- Fazer revisão terminológica em todo o relatório antes da entrega: preferir vocabulário técnico padronizado de OpenMP e programação paralela (por exemplo, conjunto/time de threads OpenMP, chunk, granularidade, escalonador, overhead, sincronização, localidade e speedup) e substituir expressões coloquiais ou ambíguas. Manter a mesma terminologia em texto, tabelas, legendas e títulos.

Não mencionar defesa, apresentação ou finalidade oral nos relatórios. Os requisitos das lições HTML da seção 1 continuam válidos.
