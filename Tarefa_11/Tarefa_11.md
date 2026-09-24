# Tarefa 11 — Particionamento de dados e balanceamento de carga

**DCA3703 · Programação Paralela · UFRN** · Prof. Samuel Xavier de Souza · Aluno: José Felix Rodrigues Anselmo

## Objetivo

Simular em 3D a difusão viscosa de um fluido (Navier-Stokes considerando **apenas** o termo viscoso, sem pressão nem forças externas) por diferenças finitas, validar o código sequencial e paralelizá-lo com OpenMP, avaliando o impacto das cláusulas `schedule` e `collapse`.

## Conceitos-chave

- **Equação simulada:** só o termo viscoso, `∂u/∂t = ν∇²u`, com ν = 0,01.
- **Esquema numérico:** diferenças finitas explícitas (estêncil de 7 pontos), com `dt = 0,4 × limite de estabilidade`.
- **Decomposição de domínio:** os dados (a malha) são fatiados entre as threads. **Decomposição funcional:** o trabalho é dividido por etapas.
- **Balanceamento estático** (`schedule(static)`): a divisão é feita uma vez, no início. Ideal quando o custo por item é uniforme.
- **Balanceamento dinâmico** (`schedule(dynamic)` e `guided`): a carga é distribuída em tempo de execução, como um saco de tarefas. Serve para problemas irregulares.
- **`collapse(n)`:** funde laços aninhados em um único espaço de iterações, gerando mais fatias para distribuir.

## O que foi feito

1. Versão **sequencial** como referência de corretude: perturbação gaussiana no centro do domínio (amplitude 2,0, largura 0,03) que se espalha e perde amplitude. Contorno com valor zero.
2. Visualização 3D dos snapshots CSV com um script Python (`visualiza.py`, não anexado ao relatório).
3. Versão **paralela** com OpenMP no laço de atualização, gerando **11 variantes**:
   - `static`, `dynamic` e `guided`, cada um com chunk 2, 4 e 8 (9 variantes);
   - `collapse(2)` e `collapse(3)`.
4. Execução com 2, 4 e 8 threads, totalizando **33 configurações**, uma execução cada.

## Como compilar e executar

```bash
# Sequencial
gcc navier_stokes.c -o navier -lm
./navier

# Paralelo (OpenMP)
gcc -fopenmp navier_stokes_p.c -o navier_stokes_collapse2 -lm
export OMP_NUM_THREADS=8
time ./navier_stokes_collapse2
```

Argumentos opcionais do programa: `NX NY NZ N_STEPS saida.csv MAX_PTS N_SNAPS` (padrão: malha 64³, 200 passos, 5 snapshots).

## Ambiente

Lenovo IdeaPad 3 15ALC6 · AMD Ryzen 5 5500U (6 núcleos / 12 threads) · 9,05 GiB de RAM · Ubuntu 26.04 LTS · GCC.
Malha 64×64×64 (0,26 milhão de pontos), 200 passos, `dt = 1,680e-03`.

## Resultados

Tempo sequencial de referência: **1,009 s**. A soma de `u` foi **212,6596807633** em todas as execuções, idêntica à da versão sequencial, então a paralelização não alterou o resultado numérico.

Tempo (s), em negrito o menor de cada coluna:

| Versão | 2 threads | 4 threads | 8 threads |
| --- | --- | --- | --- |
| static,2 | 0,631 | 0,347 | 0,300 |
| static,4 | 0,612 | 0,348 | 0,274 |
| static,8 | 0,541 | 0,299 | 0,267 |
| dynamic,2 | 0,655 | 0,339 | **0,240** |
| dynamic,4 | 0,567 | 0,315 | 0,270 |
| dynamic,8 | 0,539 | 0,313 | 0,283 |
| guided,2 | 0,523 | 0,319 | 0,260 |
| guided,4 | 0,570 | 0,326 | 0,266 |
| guided,8 | 0,531 | 0,337 | 0,281 |
| collapse(2) | **0,507** | **0,276** | 0,261 |
| collapse(3) | 0,559 | 0,298 | 0,255 |

Melhores casos (speedup = T_seq / T_par, eficiência = speedup / p):

| Threads | Melhor versão | Speedup | Eficiência |
| --- | --- | --- | --- |
| 2 | collapse(2) | 1,99 | ~100% |
| 4 | collapse(2) | 3,65 | 91% |
| 8 | dynamic,2 | 4,20 | 52% |

Os speedups de todas as configurações ficaram entre 1,54× e 4,20×.

## Conclusões

- Todas as 33 configurações foram mais rápidas que a versão sequencial.
- A eficiência cai com o número de threads: o trabalho total é fixo, e o custo de abrir a região paralela e repartir tarefas não diminui na mesma proporção.
- O custo por ponto da malha é **uniforme**, que é o caso ideal para balanceamento estático. Por isso `static` ficou competitivo. `dynamic` e `guided` não têm desequilíbrio para corrigir nessa carga.
- `collapse(2)` obteve os menores tempos com 2 e 4 threads: mais unidades de trabalho tornam a divisão mais uniforme.

## Limitações

Cada configuração foi executada **uma única vez**, então diferenças pequenas entre variantes podem ser ruído. O notebook tem 6 núcleos físicos, e com 8 threads já entram threads de SMT. Vale conferir se isso contribui para a queda de eficiência.

## Perguntas para revisão

1. Por que `schedule(static)` foi competitivo em um laço onde todos os pontos custam o mesmo?
2. Em que tipo de problema `dynamic` ou `guided` seriam claramente melhores?
3. O que `collapse(2)` muda na forma como as iterações são distribuídas entre as threads?
4. Por que a eficiência cai de ~100% (2 threads) para ~52% (8 threads)?
5. Como a igualdade da soma de `u` valida a versão paralela?
