#!/bin/bash
#SBATCH --job-name=tarefa_15
#SBATCH --time=0-0:10
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks=2
#SBATCH --cpus-per-task=1
#SBATCH --output=%x_%j.out
#SBATCH --error=%x_%j.err

mpicc -o barra barra1d.c
mpirun -np 2 ./barra 1 1000000 1000   # Send/Recv
mpirun -np 2 ./barra 2 1000000 1000   # Isend/Irecv+Wait
mpirun -np 2 ./barra 3 1000000 1000   # Isend/Irecv+Test  (já tem)