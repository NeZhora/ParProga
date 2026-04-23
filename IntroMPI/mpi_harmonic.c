/* mpi_harmonic.c
 *
 * Параллельное вычисление частичной суммы гармонического ряда:
 *   S = sum_{n=1}^{N} 1/n
 *
 * N передаётся как аргумент командной строки.
 * Каждый процесс считает свою часть суммы, затем результаты собираются
 * на процессе с rank=0 с помощью MPI_Reduce.
 *
 * Компиляция: mpicc -o mpi_harmonic mpi_harmonic.c -lm
 * Запуск:     mpirun -n 4 ./mpi_harmonic 1000000
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int rank, size;
    long long N;           /* Верхний предел суммирования */
    double local_sum = 0.0; /* Локальная (частичная) сумма на каждом процессе */
    double global_sum = 0.0; /* Итоговая сумма (только на rank=0) */

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Считываем N из аргументов командной строки */
    if (argc < 2) {
        if (rank == 0) {
            fprintf(stderr, "Использование: mpirun -n <np> ./mpi_harmonic <N>\n");
        }
        MPI_Finalize();
        return 1;
    }
    N = atoll(argv[1]);

    /* Распределяем работу между процессами.
     * Процесс с рангом rank обрабатывает индексы: rank+1, rank+1+size, rank+1+2*size, ...
     * Это циклическое (round-robin) распределение — простое и достаточно равномерное. */
    long long i;
    for (i = rank + 1; i <= N; i += size) {
        local_sum += 1.0 / (double)i;
    }

    /* MPI_Reduce собирает значения local_sum со всех процессов,
     * применяет операцию MPI_SUM и помещает результат в global_sum на процессе 0.
     * Это коллективная операция — все процессы должны её вызвать. */
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    /* Только мастер-процесс (rank=0) выводит результат */
    if (rank == 0) {
        printf("N = %lld\n", N);
        printf("Сумма гармонического ряда S(%lld) = %.15f\n", N, global_sum);
        printf("Число процессов: %d\n", size);
    }

    MPI_Finalize();
    return 0;
}