#!/bin/sh
#SBATCH -n 16                         # Максимум 16 процессов на кластере
#SBATCH -o benchmarkBIG-%j.out
#SBATCH -e benchmarkBIG-%j.err

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
echo "=== Последовательная версия ==="
./transport_seq 1 $M $K