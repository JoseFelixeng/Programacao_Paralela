#!/bin/bash
#SBATCH --job-name=tarefa_15_scaling
#SBATCH --time=0-0:30
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks=32
#SBATCH --cpus-per-task=1
#SBATCH --output=%x_%j.out
#SBATCH --error=%x_%j.err

# Compilar com otimizacao -- sem -O3 o tempo de computacao fica maior,
# distorcendo a proporcao comunicacao/computacao que estamos comparando.
mpicc -O3 -march=native -o barra barra1d.c -lm

N=1000000
STEPS=1000
REPS=3          # repeticoes por combinacao, para filtrar ruido do cluster compartilhado

for P in 2 4 8 16 32; do
  for MODO in 1 2 3; do
    for REP in $(seq 1 $REPS); do
      echo "--- P=$P MODO=$MODO REP=$REP ---"
      mpirun -np $P ./barra $MODO $N $STEPS
    done
  done
done