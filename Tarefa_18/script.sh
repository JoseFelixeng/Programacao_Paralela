#!/bin/bash
#SBATCH --partition=amd-512
#SBATCH --nodes 1
#SBATCH --time 00:30:00
#SBATCH --job-name vadd
#SBATCH --output vadd_p-%j.out

cd $SLURM_SUBMIT_DIR

gcc -g -Wall -fopenmp vadd_p.c -o vadd_p

ulimit -s unlimited

./vadd_p
