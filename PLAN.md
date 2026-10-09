# Plano — Tarefa 13: avaliação da escalabilidade com afinidade de threads

## Contexto

A Tarefa 13 deve medir, no mesmo nó do NPAD usado na Tarefa 12, como a escalabilidade do solver de Navier–Stokes varia com as afinidades de threads oferecidas pelo OpenMP/sistema operacional. O objetivo é atender estritamente ao enunciado, reutilizando o código e a infraestrutura já existentes em `tarefa12/` e evitando novas variantes do solver sem necessidade.

Estado encontrado:

- `tarefa12/` já contém quatro versões do solver, coleta PaScal, dados e relatório.
- O job 2106246 da Tarefa 12 executou em `r2n04.npad.internal`: 2 × AMD EPYC 7713, 128 núcleos físicos, SMT2, 256 CPUs lógicas e 8 domínios NUMA. A campanha usou até 64 threads, `OMP_PLACES=cores` e `OMP_PROC_BIND=spread`.
- Portanto, `tarefa12/runs/data/v4_numa.json` já é a medição `spread_cores`. Ela **não será reexecutada, carregada no Viewer da Tarefa 13 nem incluída no novo relatório**. O hash registrado de `v4_numa.c` (`b2ae88e3dc229a892a93e079c1b175365088ed87ce6fec37b91bb9f70b77682f`) coincide com o arquivo atual.
- A versão `tarefa12/v4_numa.c` é a base mais adequada: mantém a evolução otimizada e inicializa os dois campos em paralelo, aplicando *first touch* coerente com a afinidade.
- Ainda não existe `tarefa13/`.
- `lessons/0013-tarefa13-afinidade-numa.html` e os índices já têm conteúdo preliminar, mas ainda dizem que a medição está pendente e apontam para `tarefa12/`.
- Os slides do professor listam `false`, `true`, `close`, `spread` e `master`, além de `OMP_PLACES`; para o escopo mínimo escolhido, será usado o subconjunto comparável `false`, `close` e `spread`, incluindo `cores` e `threads`.
- Há alterações locais não relacionadas; a implementação deverá preservá-las.

## Abordagem

Preparar para o usuário executar uma campanha **somente com PaScal Analyzer/Viewer**, igual à Tarefa 12, usando `tarefa12/v4_numa.c`. Manter fixos o nó `r2n04`, as sete cargas, 500 passos, compilador e flags; variar apenas:

1. número de threads;
2. política de afinidade OpenMP (`OMP_PROC_BIND`);
3. definição de lugares (`OMP_PLACES`).

As quatro configurações novas serão tratadas no Viewer como quatro “versões” do mesmo executável:

- `false`: `OMP_PROC_BIND=false`, sem `OMP_PLACES` relevante;
- `close_cores`: `close` + `cores`;
- `close_threads`: `close` + `threads`;
- `spread_threads`: `spread` + `threads`.

Cada configuração repetirá exatamente a matriz da Tarefa 12: threads `1,2,4,8,16,32,64` × cargas `262144,524288,1048576,2097152,4194304,8388608,16777216`, com uma repetição, totalizando 196 pontos novos. O binário será compilado uma vez com a instrumentação manual PaScal e copiado/renomeado para que os quatro JSONs apareçam separadamente, preservando hashes idênticos.

A análise será feita exclusivamente no PaScal Viewer a partir desses quatro JSONs novos. Não haverá CSV próprio, cálculo externo nem script Python. O relatório conterá **exatamente quatro gráficos**: o gráfico `Scalability — Whole Program (Region 0)` exportado pelo Viewer para cada configuração nova. Nenhum gráfico, tabela de resultados ou interpretação da execução `spread_cores` da Tarefa 12 será repetido.

O usuário executará a campanha no NPAD e abrirá os JSONs no Viewer depois que os artefatos forem preparados; o relatório final e a lição só receberão conclusões após os gráficos reais retornarem.

## Arquivos a modificar

- `tarefa13/README.md` — instruções curtas para submissão e uso dos cinco JSONs no PaScal Viewer.
- `tarefa13/afinidade.slurm` — compilação PaScal, captura de proveniência e quatro chamadas do Analyzer com a matriz da Tarefa 12.
- `tarefa13/runs/pascal-<job>/data/*.json` — quatro resultados novos do PaScal Analyzer, incorporados depois da execução no NPAD.
- `tarefa13/images/*.png` — exatamente quatro gráficos `Scalability — Whole Program (Region 0)`, um por configuração nova, exportados do PaScal Viewer.
- `tarefa13/relatorio.tex` e `tarefa13/relatorio.pdf` — relatório técnico compacto, finalizado após a análise no Viewer.
- `lessons/0013-tarefa13-afinidade-numa.html` — substituir o estado pendente pela prática e pelos resultados reais no documento único obrigatório.
- `README.md` — trocar o estado “medição pendente” pelos artefatos finais.
- `index.html` — adicionar links para execução e relatório da Tarefa 13.

## Reuso

- Solver `tarefa12/v4_numa.c`, sem criar nova variante numérica.
- Flags e macros PaScal de `tarefa12/build.sh`: C11, OpenMP, `-O2`, avisos estritos, `-ffp-contract=off`, `USE_PASCAL` e `WORKLOAD_INPUT`.
- Matriz e invocação do Analyzer de `tarefa12/pascal.slurm`: `-t man`, sete contagens de threads e sete cargas.
- Captura de proveniência de `tarefa12/pascal.slurm`: `hostname`, `lscpu`, `scontrol show job`, ajuda do Analyzer e hashes.
- Estrutura dos JSONs e exportações do PaScal Viewer já usada em `tarefa12/runs/data/` e `tarefa12/images/`.
- Estrutura e estilo de `tarefa12/relatorio.tex` e `assets/scientific_report.sty`.
- Guia do ambiente em `assets/guia-comandos-npad.md`.
- Material preliminar em `lessons/0013-tarefa13-afinidade-numa.html`.

## Passos

- [x] Criar a estrutura mínima de `tarefa13/` sem duplicar o solver da Tarefa 12.
- [x] Implementar o job fixado em `r2n04`, compilando uma vez o executável PaScal e expondo quatro nomes para as configurações ainda não medidas, com hashes idênticos.
- [x] Reutilizar em cada nova configuração a matriz PaScal da Tarefa 12, alterando apenas `OMP_PROC_BIND` e `OMP_PLACES`; não reexecutar `spread_cores`.
- [x] Fazer o job recusar diretório de saída reutilizado e registrar código, hashes, topologia, cpuset, variáveis OpenMP e afinidade observada.
- [x] Executar localmente apenas compilação sem PaScal e revisão estática do script; resultados locais não entram no trabalho.
- [x] Entregar ao usuário o comando único de submissão no NPAD e aguardar os quatro JSONs novos.
- [x] Abrir somente os quatro JSONs novos no PaScal Viewer e exportar um gráfico `Scalability — Whole Program (Region 0)` para cada configuração.
- [x] Incorporar os quatro gráficos reais e redigir o relatório técnico compacto somente sobre as novas execuções, distinguindo medição de hipótese.
- [x] Atualizar a lição HTML única e os três índices obrigatórios.
- [x] Revisar links, terminologia e ausência de artefatos de build versionados.

## Verificação

- Compilar `tarefa12/v4_numa.c` com as mesmas flags/macros PaScal da Tarefa 12 no NPAD.
- Executar uma validação curta para confirmar que as cinco configurações são aceitas no nó `r2n04`.
- Confirmar a associação efetiva com `OMP_DISPLAY_ENV=VERBOSE`, `OMP_DISPLAY_AFFINITY=TRUE`, cpuset do job e `numactl --hardware` quando disponível.
- Confirmar quatro JSONs novos válidos, cada um com 49 combinações, uma repetição, mesmas sete cargas e mesmos 500 passos.
- Conferir nos metadados dos quatro JSONs o mesmo hostname, programa-base e matriz, mudando apenas a configuração de afinidade; verificar também o hash do código contra o registro da Tarefa 12.
- Abrir somente os quatro JSONs novos no PaScal Viewer e verificar os quatro gráficos `Scalability — Whole Program (Region 0)`.
- Confirmar que o relatório não inclui gráfico, tabela de resultados nem reanálise de `spread_cores` da Tarefa 12.
- Não usar cálculo, tabela ou gráfico externo para reinterpretar os JSONs.
- Recompilar o LaTeX e inspecionar o PDF final.
- Validar links relativos da lição e links do GitHub Pages no `README.md`.
- Confirmar que o diff não altera nem sobrescreve mudanças locais preexistentes.
