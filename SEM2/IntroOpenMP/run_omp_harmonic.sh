#!/bin/sh
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH -o omp_harmonic-%j.out
#SBATCH -e omp_harmonic-%j.err

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./omp_harmonic 10000000
