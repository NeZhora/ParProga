#!/bin/sh
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH -o omp_seq-%j.out
#SBATCH -e omp_seq-%j.err

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./omp_sequential
