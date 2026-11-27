# Вводные задачи по OpenMP

1. `omp_hello` — каждый поток печатает свой номер и общее число потоков. Порядок строк может меняться между запусками.
2. `omp_harmonic` — считает сумму `1 + 1/2 + … + 1/N`. Положительное целое `N` передаётся первым аргументом. Главный поток складывает частичные суммы в порядке номеров потоков. При смене числа потоков последние цифры могут отличаться из-за округления.
3. `omp_sequential` — потоки по очереди увеличивают общий счётчик в порядке своих номеров. Итоговое значение равно числу потоков.

## Сборка

На Linux или на головном узле кластера перейдите из корня репозитория в `SEM2/IntroOpenMP` и соберите программы с помощью GCC:

```sh
cd SEM2/IntroOpenMP
gcc -O2 -Wall -Wextra -fopenmp omp_hello.c -o omp_hello
gcc -O2 -Wall -Wextra -fopenmp omp_harmonic.c -o omp_harmonic
gcc -O2 -Wall -Wextra -fopenmp omp_sequential.c -o omp_sequential
```

## Запуск на кластере

Из той же папки отправьте задания в Slurm:

```sh
sbatch run_omp_hello.sh
sbatch run_omp_harmonic.sh
sbatch run_omp_seq.sh
squeue --me
```

Каждый скрипт запрашивает один узел и восемь CPU для одного процесса. Число потоков берётся из `SLURM_CPUS_PER_TASK`. Например, для четырёх потоков используйте `sbatch --cpus-per-task=4 run_omp_harmonic.sh`. В скрипте для суммы задано `N=10000000`.

Slurm печатает номер задания после `sbatch`. Результат появится в `omp_hello-<id>.out`, `omp_harmonic-<id>.out` или `omp_seq-<id>.out`. Сообщения об ошибках попадут в одноимённый файл с расширением `.err`.

## Локальная проверка

```sh
OMP_NUM_THREADS=4 ./omp_hello
OMP_NUM_THREADS=4 ./omp_harmonic 10
OMP_NUM_THREADS=4 ./omp_sequential
```

Для `N=10` ожидаемая сумма примерно `2.928968253968254`. При четырёх потоках последовательный доступ завершится строкой `Final value = 4`. Для `omp_harmonic` задавайте не более 256 потоков, поскольку под частичные суммы выделено 256 ячеек.
