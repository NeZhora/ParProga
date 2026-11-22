#!/bin/sh
#SBATCH -N 1                          # 1 узел (потоки работают на одном узле!)
#SBATCH --ntasks-per-node=1           # 1 задача (1 процесс)
#SBATCH --cpus-per-task=8             # 8 ядер для потоков
#SBATCH -o pthread_hello-%j.out
#SBATCH -e pthread_hello-%j.err
./pthread_hello