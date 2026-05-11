#!/bin/sh
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH -o pthread_seq-%j.out
#SBATCH -e pthread_seq-%j.err
./pthread_sequential