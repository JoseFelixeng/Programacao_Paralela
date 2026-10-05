#!/bin/bash
#SBATCH --partition=amd-512
#SBATCH --nodes 1
#SBATCH --time 00:30:00
#SBATCH --job-name heat
#SBATCH --output heat1-%j.out

cd $SLURM_SUBMIT_DIR

gcc -g -Wall -fopenmp heat.c -o heat1 -lm 

ulimit -s unlimited

./heat1 8000 10 