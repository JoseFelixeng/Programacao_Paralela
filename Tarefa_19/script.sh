#!/bin/bash
#SBATCH --partition=amd-512
#SBATCH --gpus-per-node=2 
#SBATCH --nodes 1
#SBATCH --time 00:02:00
#SBATCH --job-name vadd
#SBATCH --output vadd-%j.out

gcc -g -Wall -fopenmp vadd_s.c -o  vadd_s
ulimit -s unlimited

./vadd_s