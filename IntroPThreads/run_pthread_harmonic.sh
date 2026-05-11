#!/bin/sh
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH -o pthread_harmonic-%j.out
#SBATCH -e pthread_harmonic-%j.err
./pthread_harmonic 10000000