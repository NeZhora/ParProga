#!/bin/sh
#SBATCH -n 16                         # Максимум 16 процессов на кластере
#SBATCH -o integralMPI-%j.out
#SBATCH -e integralMPI-%j.err

EPS="1e-8"

echo "=== Бенчмарк ускорения ==="
echo "Интервал: [0.04; 12], Точность eps=$EPS"
echo ""

# Запускаем с разным числом процессов
for np in 1 2 4 8 16; do
    echo "--- np = $np ---"
    mpirun -np $np ./integralMPI $EPS
    echo ""
done