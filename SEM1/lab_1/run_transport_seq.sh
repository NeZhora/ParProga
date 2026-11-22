#!/bin/sh
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH -o transport_seq-%j.out
#SBATCH -e transport_seq-%j.err

# Аргументы: <номер_схемы> <M> <K>
# Схема 1 (левый уголок), M=1000 узлов по x, K=2000 узлов по t
./transport_seq 1 1000 2000