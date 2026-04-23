
### Задача 3: Круговая пересылка (MPI)

/* mpi_ring.c
 *
 * Круговая пересылка сообщения между MPI-процессами.
 * Переменная типа int передаётся от процесса 0 → 1 → 2 → ... → (size-1) → 0.
 * Каждый процесс инкрементирует значение на 1 и выводит информацию.
 *
 * Компиляция: mpicc -o mpi_ring mpi_ring.c
 * Запуск:     mpirun -n 4 ./mpi_ring
 */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int rank, size;
    int value;          /* Передаваемое значение */
    int tag = 0;        /* Метка сообщения */
    MPI_Status status;  /* Статус приёма сообщения */

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        /* Процесс 0 инициирует пересылку: задаёт начальное значение */
        value = 0;
        printf("[Rank %d] Начальное значение: %d\n", rank, value);

        /* Инкрементируем значение */
        value++;
        printf("[Rank %d] После инкремента: %d. Отправляю процессу %d\n",
               rank, value, rank + 1);

        /* Отправляем следующему процессу (rank+1) */
        MPI_Send(&value, 1, MPI_INT, rank + 1, tag, MPI_COMM_WORLD);

        /* Ждём получения сообщения от последнего процесса (замыкание кольца) */
        MPI_Recv(&value, 1, MPI_INT, size - 1, tag, MPI_COMM_WORLD, &status);
        printf("[Rank %d] Получил обратно значение: %d (прошло полный круг)\n",
               rank, value);

    } else {
        /* Все остальные процессы: сначала принимаем от предыдущего */
        MPI_Recv(&value, 1, MPI_INT, rank - 1, tag, MPI_COMM_WORLD, &status);
        printf("[Rank %d] Получил значение: %d от процесса %d\n",
               rank, value, rank - 1);

        /* Инкрементируем */
        value++;
        printf("[Rank %d] После инкремента: %d\n", rank, value);

        /* Определяем следующего получателя: для последнего процесса это 0 */
        int next = (rank + 1) % size;

        /* Отправляем следующему */
        MPI_Send(&value, 1, MPI_INT, next, tag, MPI_COMM_WORLD);
        printf("[Rank %d] Отправил значение %d процессу %d\n",
               rank, value, next);
    }

    MPI_Finalize();
    return 0;
}