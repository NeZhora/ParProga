#!/bin/sh
#SBATCH -n 8                          # 8 MPI-процессов
#SBATCH -o transport_mpi-%j.out
#SBATCH -e transport_mpi-%j.err

# Аргументы: <M> <K>
mpirun -np 8 ./transport_mpi 10000 10000