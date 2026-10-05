#!/bin/bash
#SBATCH --partition=amd-512
#SBATCH --nodes 1
#SBATCH --time 00:30:00
#SBATCH --job-name heat
#SBATCH --output heat-%j.out

cd $SLURM_SUBMIT_DIR

gcc -g -Wall -fopenmp heat.c -o heat -lm 

ulimit -s unlimited

./heat 8000 10 