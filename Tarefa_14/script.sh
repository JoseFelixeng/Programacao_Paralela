#!/bin/bash
#SBATCH --partition=amd-512 #partição para a qual o job é enviado
#SBATCH --ntasks=2
#SBATCH --time=0-0:5

mpirun main