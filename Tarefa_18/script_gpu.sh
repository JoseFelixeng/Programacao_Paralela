#!/bin/bash
#SBATCH --partition=gpu-8-v100 
#SBATCH --gpus-per-node=1
#SBATCH --nodes 1
#SBATCH --time 00:30:00
#SBATCH --job-name vadd_gpu
#SBATCH --output vadd_gpu-%j.out

cd $SLURM_SUBMIT_DIR
ulimit -s unlimited

module load compilers/nvidia/nvhpc/24.11

nvidia-smi
export OMP_TARGET_OFFLOAD=MANDATORY
./vadd_gpu 