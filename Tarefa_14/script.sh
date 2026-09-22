#!/bin/bash
#SBATCH --job-name=tarefa_14
#SBATCH --time=0-0:10
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks=2
#SBATCH --cpus-per-task=1
#SBATCH --output=%x_%j.out
#SBATCH --error=%x_%j.err

mpicc -o main main.c
mpirun main 