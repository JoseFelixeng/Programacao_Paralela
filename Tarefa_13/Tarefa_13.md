# Tarefa 13 — Arquiteturas paralelas (afinidade de threads)

**DCA3703 · Programação Paralela · UFRN** · Prof. Samuel Xavier de Souza · Aluno: José Felix Rodrigues Anselmo

## Objetivo

Reavaliar a escalabilidade do código de Navier-Stokes (difusão viscosa 3D) no NPAD, agora investigando o efeito da **afinidade de threads** (`OMP_PROC_BIND` e `OMP_PLACES`), e comentar escalabilidade forte e fraca.

## Conceitos-chave

- **`OMP_PLACES`:** define os "lugares" do hardware onde as threads podem rodar (`cores`, `threads` ou listas explícitas).
- **`OMP_PROC_BIND=close`:** threads criadas próximas da thread mestre.
- **`OMP_PROC_BIND=spread`:** threads espalhadas pelos lugares disponíveis.
- **NUMA:** em nós com vários domínios de memória, onde a thread roda importa para o tráfego de memória.
- **Kernel memory-bound:** o estêncil FTCS de 7 pontos tem baixa intensidade aritmética por byte acessado, então o limite é a largura de banda de memória.

## Ambiente

NPAD, partição `amd-512`, 1 nó exclusivo, 32 núcleos. PaScal Analyzer com núcleos `1,2,4,8,16,32` e entradas `10,20,40,80,160,320`.
Código base: a **v3 da Tarefa 12** (gaussiana paralelizada e instrumentada com `pascal_start(2)`).

O script é o da Tarefa 12, acrescentando apenas as variáveis de afinidade:

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
export OMP_PROC_BIND=close      # varia por teste
export OMP_PLACES=cores         # varia por teste
gcc -fopenmp navier_stokes_p.c -lmpascalops -o navier_stokes_p_v0 -lm
pascalanalyzer ./navier_stokes_p_v0 -t man -g -r 5 --idtm 1 -c 1,2,4,8,16,32 -i 10,20,40,80,160,320 -v INFO -o navier_stokes_v0.json
```

## Testes de afinidade

| Teste | `OMP_PROC_BIND` | `OMP_PLACES` |
| --- | --- | --- |
| 1 | `close` | `cores` |
| 2 | `spread` | `cores` |
| 3 | `spread` | `"{0:16:1},{16:16:1}"` (dois grupos de 16 núcleos) |

Para cada teste foram gerados os sete painéis do PaScal Viewer: Efficiency, Performance, Scalability, Speedup, Speedup Loss, Strong Scalability e Weak Scalability.

## Resultados

- Os três testes tiveram gráficos **praticamente idênticos**.
- Speedup máximo de cerca de **14×** (sobre um ideal de 32×), com 32 threads na maior entrada (i6).
- A perda de speedup atinge o pico (perto de 29–30) com 32 threads nas menores entradas (i1–i3), em todos os testes.
- **Única diferença:** no Teste 3, com 2 threads nas entradas i3 e i4, o mapa de Escalabilidade ficou negativo (vermelho), enquanto nos Testes 1 e 2 essas células ficaram positivas.

## Conclusões

- A política de afinidade **não alterou de forma relevante** a escalabilidade. O gargalo não está em como as threads são distribuídas.
- É coerente com um kernel memory-bound em nó exclusivo: `close` e `spread` acabam saturando a mesma largura de banda de memória agregada.
- A anomalia do Teste 3 tem uma explicação plausível (mas não confirmada): com dois grupos fixos, já com 2 threads uma fica em cada grupo, possivelmente em domínios de memória distintos, gerando tráfego entre eles. Forçar `OMP_PLACES` manualmente sem conferir a topologia real (por exemplo, com `numactl -H`) pode causar pequenas penalidades sem ganho.

## Perguntas para revisão

1. O que é um kernel *memory-bound* e por que a afinidade de threads pouco o afeta?
2. Qual a diferença prática entre `close` e `spread`?
3. Por que `OMP_PLACES=cores` deixa as threads nos núcleos físicos, e o que mudaria com `threads`?
4. Como investigar a topologia NUMA do nó antes de fixar `OMP_PLACES`?
5. Por que o resultado negativo do Teste 3 aparece só com 2 threads?
