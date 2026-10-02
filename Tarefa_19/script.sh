#!/bin/bash
#SBATCH --partition=gpu-4-a100 
#SBATCH --gpus-per-node=2 
#SBATCH --nodes 1
#SBATCH --time 00:30:00
#SBATCH --job-name heat
#SBATCH --output heat-%j.out

gcc -g -Wall -fopenmp heat.c -o  heat

ulimit -s unlimited

./heat