#!/bin/sh
#SBATCH -n 2                          # Ровно 2 процесса для ping-pong
#SBATCH -o latency-%j.out
#SBATCH -e latency-%j.err
mpirun -np 2 ./mpi_latency