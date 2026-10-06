#!/bin/bash
#SBATCH --partition=gpu-8-v100 
#SBATCH --gpus-per-node=1
#SBATCH --nodes 1
#SBATCH --time 00:30:00
#SBATCH --job-name heat_gpu
#SBATCH --output heat_gpu1-%j.out

cd $SLURM_SUBMIT_DIR
ulimit -s unlimited

module load compilers/nvidia/nvhpc/24.11

nvc -mp=gpu -gpu=cc70 -O3 -Minfo=mp -o heat_gpu1 heat_gpu -lm

nvidia-smi
export OMP_TARGET_OFFLOAD=MANDATORY

nsys profile --stats=true -o nsys_heat_gpu2 ./heat_gpu1 8000 1000