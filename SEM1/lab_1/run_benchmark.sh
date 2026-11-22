#!/bin/sh
#SBATCH -n 16                         # Максимум 16 процессов на кластере
#SBATCH -o benchmark-%j.out
#SBATCH -e benchmark-%j.err

M=10000
K=10000

echo "=== Бенчмарк ускорения ==="
echo "M=$M, K=$K"
echo ""

# Запускаем с разным числом процессов
for np in 1 2 4 8 16; do
    echo "--- np = $np ---"
    mpirun -np $np ./transport_mpi $M $K
    echo ""
done