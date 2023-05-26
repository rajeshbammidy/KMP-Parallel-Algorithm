#!/bin/bash

#SBATCH --nodes=64
#SBATCH --ntasks-per-node=1
#SBATCH --constraint=AVX512
#SBATCH --time=00:25:00
#SBATCH --partition=general-compute
#SBATCH --qos=general-compute
#SBATCH --mail-type=END
#SBATCH --mail-user=rbammidi@buffalo.edu
#SBATCH --job-name="KMP-Parallel"
#SBATCH --output=./kmp.out
#SBATCH --exclusive 


module load intel
module list

export I_MPI_DEBUG=4
export I_MPI_PMI_LIBRARY=/opt/software/slurm/lib64/libpmi.so

mpicc -o kmp kmp.c
srun -n 64 kmp

