#!/bin/bash
#SBATCH --job-name=tarefa16
#SBATCH --partition=amd-512
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --ntasks=32
#SBATCH --time=00:30:00
#SBATCH --output=tarefa16_%j.out
#SBATCH --error=tarefa16_%j.err



set -e

mpicc -O3 -o multMxV multMxV.c -lm

REPS=30
OUT="resultados_tarefa16_${SLURM_JOB_ID}.csv"
echo "nprocs,M,N,reps,tempo_medio_s" > "$OUT"

TAMANHOS=(512 1024 2048 4096 8192)
NPROCS_LIST=(1 2 4 8 16 32)

for M in "${TAMANHOS[@]}"; do
    N=$M
    for P in "${NPROCS_LIST[@]}"; do
        if [ $((M % P)) -ne 0 ]; then
            continue
        fi
        mpirun -np "$P" ./multMxV "$M" "$N" "$REPS" >> "$OUT"
    done
done

echo "Concluido. Resultados em $OUT"


