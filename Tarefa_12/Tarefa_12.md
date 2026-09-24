# Tarefa 12 — Avaliação de desempenho e escalabilidade paralela

**DCA3703 · Programação Paralela · UFRN** · Prof. Samuel Xavier de Souza · Aluno: José Felix Rodrigues Anselmo

## Objetivo

Avaliar a escalabilidade do código de difusão viscosa 3D (subconjunto de Navier-Stokes) em um nó do NPAD, identificar gargalos e reportar a evolução em versões sucessivas, comentando escalabilidade geral, **forte** e **fraca**.

## Conceitos-chave

| Conceito | Definição |
| --- | --- |
| Desempenho | 1 / Tempo |
| Eficiência | Trabalho / (Tempo × Recursos) |
| Speedup (Amdahl, problema fixo) | S(p) = T(1) / T(p) |
| Eficiência (Gustafson, problema cresce) | E(p) = S(p) / p |
| Escalabilidade forte | Tamanho do problema fixo, núcleos crescendo |
| Escalabilidade fraca | Núcleos e tamanho do problema crescendo juntos |

- **Lei de Amdahl:** a fração serial fixa impõe um teto ao speedup. Acelerar só a parte paralela reduz esse teto.
- Programas **escaláveis** compensam a perda de eficiência aumentando o problema junto com os recursos.

## Ambiente e ferramentas

- NPAD, partição `amd-512`, 1 nó exclusivo, 32 núcleos.
- **PaScal Analyzer** (`pascalanalyzer`) e PaScal Viewer, com instrumentação `pascal_start(n)` / `pascal_stop(n)`.
- Combinações testadas: núcleos `1,2,4,8,16,32` × entradas `10,20,40,80,160,320` (dobrando juntos, para ler a escalabilidade fraca na diagonal), 5 repetições.

```bash
#!/bin/bash
#SBATCH --job-name=tarefa_12
#SBATCH --time=0-0:30
#SBATCH --partition=amd-512
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --cpus-per-task=32
#SBATCH --output=%x_%j.out

cd ~/Tarefa_12
source ~/pascal-releases-master/env.sh
gcc -O3 -march=native -fopenmp navier_stokes_p.c -lmpascalops -o navier_stokes_p_v1 -lm
pascalanalyzer ./navier_stokes_p_v1 -t man -g -r 5 --idtm 1 -c 1,2,4,8,16,32 -i 10,20,40,80,160,320 -v INFO -o navier_stokes_v1.json
```

## Versões avaliadas

| Versão | O que muda | Speedup máximo observado |
| --- | --- | --- |
| **v0** | Só o laço de difusão paralelizado (`collapse(2)`), sem flags de otimização | ~12× |
| **v1** | Mesmo código da v0, compilado com `-O3 -march=native` | ~2× |
| **v2** | Paraleliza também a gaussiana (`collapse(3)`), **sem** instrumentá-la no PaScal | ~12× |
| **v3** | Igual à v2, com `pascal_start(2)`/`pascal_stop(2)` na gaussiana | (análise por região) |

## Resultados

- **v0:** eficiência perto de 100% com poucos núcleos e malha grande, caindo para perto de 0% com 32 núcleos na menor malha (10³ pontos), onde o overhead de criar e sincronizar threads domina.
- **v1:** o teto de speedup cai de ~12× para ~2×. A otimização do compilador acelerou a parte paralela, então a fração serial fixa (soma final, gravação do CSV, criação de threads a cada passo) passou a dominar. É Amdahl na prática: **mais rápido, mas menos escalável**.
- **v2:** teto volta a ~12×, porque a região medida continua sendo só a difusão. A gaussiana paralelizada fica invisível para a métrica.
- **v3:** a árvore de regiões do PaScal mostra que a gaussiana (região 0.2) ocupa entre 0% e 4% do tempo total, contra 2% a 46% da difusão (0.1) e 1% a 49% somados nas regiões seriais 0.p1/0.p2/0.p3.

## Conclusões

- **Escalabilidade geral:** o código é escalável, mas de forma limitada. Nenhuma versão mantém 100% de eficiência ao crescer os núcleos.
- **Forte:** a eficiência degrada com os núcleos em todas as versões, mais nas malhas pequenas.
- **Fraca:** mais estável na diagonal, mas com degradação visível nos maiores números de núcleos, principalmente na v1.
- **Gargalo:** não é qual função é paralelizada. É a estrutura do laço principal: `#pragma omp parallel` dentro de `passo_difusao` recria as threads a cada um dos 200 passos, e o trecho serial que sobra (soma de verificação, snapshots) tem custo fixo. Paralelizar a gaussiana ganharia no máximo ~4%.

## Pendências

- Confirmar se a **v2 foi recompilada com `-O3 -march=native`**. O teto de ~12× coincide com o da v0 (sem flags) e não com o da v1, o que sugere que não. O próprio relatório marca isso para conferir antes da apresentação.

## Perguntas para revisão

1. Por que uma otimização de compilador pode *piorar* o teto de speedup?
2. Como ler escalabilidade forte e fraca em um mapa de calor do PaScal (colunas vs. diagonal)?
3. Por que paralelizar a gaussiana não resolve o gargalo? O que a região 0.2 mostra?
4. O que muda se o `#pragma omp parallel` sair de dentro do laço de passos de tempo?
5. Qual a diferença entre o modelo de Amdahl e o de Gustafson?
