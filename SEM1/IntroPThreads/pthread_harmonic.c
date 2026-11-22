/* Параллельное вычисление частичной суммы гармонического ряда:
 *   S = sum_{n=1}^{N} 1/n
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 4

/* Структура для передачи данных в поток и получения результата */
typedef struct {
    int thread_id;
    int total_threads;
    long long N;          /* Верхний предел суммирования */
    double partial_sum;   /* Результат — частичная сумма этого потока */
} thread_data_t;

/* Функция потока: вычисляет свою часть суммы */
void *compute_sum(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    double sum = 0.0;
    long long i;

    /* Циклическое распределение: поток с номером tid обрабатывает
     * индексы tid+1, tid+1+total, tid+1+2*total, ... */
    for (i = data->thread_id + 1; i <= data->N; i += data->total_threads) {
        sum += 1.0 / (double)i;
    }

    data->partial_sum = sum;
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Использование: ./pthread_harmonic <N>\n");
        return 1;
    }

    long long N = atoll(argv[1]);
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    int i;

    /* Создаём потоки */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].total_threads = NUM_THREADS;
        thread_data[i].N = N;
        thread_data[i].partial_sum = 0.0;

        pthread_create(&threads[i], NULL, compute_sum, (void *)&thread_data[i]);
    }

    /* Собираем результаты */
    double total_sum = 0.0;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_sum += thread_data[i].partial_sum;
    }

    printf("N = %lld\n", N);
    printf("Сумма гармонического ряда S(%lld) = %.15f\n", N, total_sum);
    printf("Число потоков: %d\n", NUM_THREADS);

    return 0;
}