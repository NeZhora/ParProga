#!/bin/sh
#SBATCH -n 4                      # Запрашиваем 4 MPI-процесса
#SBATCH -o mpi_hello-%j.out       # Файл для стандартного вывода (%j = номер задачи)
#SBATCH -e mpi_hello-%j.err       # Файл для ошибок
mpirun -np 4 ./mpi_hello