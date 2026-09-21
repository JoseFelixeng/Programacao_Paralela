#SBATCH --time=0-0:30
#SBATCH --partition=amd-512
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --cpus-per-task=32
#SBATCH --output=%x_%j.out

cd ~/Tarefa_12
source ~/pascal-releases-master/env.sh
export OMP_PROC_BIND=false
gcc  -fopenmp navier_stokes_p.c -lmpascalops -o navier_stokes_p_v0 -lm
pascalanalyzer ./navier_stokes_p_v0 -t man -g -r 5 --idtm 1 -c 1,2,4,8,16,32 -i 10,20,40,80,160,320 -v INFO -o navier_stokes_v0.json




