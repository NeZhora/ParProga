#!/bin/sh
#SBATCH -n 8                          # 8 MPI-процессов
#SBATCH -o mpi_harmonic-%j.out
#SBATCH -e mpi_harmonic-%j.err
mpirun -np 8 ./mpi_harmonic 10000000
~                                      