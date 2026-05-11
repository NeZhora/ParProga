#!/bin/sh
#SBATCH -n 6                      # 6 процессов в кольце
#SBATCH -o mpi_ring-%j.out
#SBATCH -e mpi_ring-%j.err
mpirun -np 6 ./mpi_ring