#!/bin/bash
#SBATCH --partition=gpu-4-a100 
#SBATCH --gpus-per-node=2 
#SBATCH --nodes 1
#SBATCH --time 00:02:00
#SBATCH --job-name vadd
#SBATCH --output vadd-%j.out

gcc -g -Wall -fopenmp vadd_p.c -o  vadd_p

ulimit -s unlimited

./vadd_p