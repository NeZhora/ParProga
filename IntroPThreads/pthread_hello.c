/* Каждый поток выводит "Hello World", свой номер и общее число потоков. */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* Общее число потоков (задаётся в коде) */
#define NUM_THREADS 4

/* Структура для передачи данных в поток */
typedef struct {
    int thread_id;    /* Номер потока (задаётся вручную) */
    int total_threads; /* Общее число потоков */
} thread_data_t;

/* Функция, выполняемая каждым потоком */
void *hello_func(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;

    /* Выводим приветствие с номером потока и общим числом потоков.
     * Порядок вывода не детерминирован — потоки работают параллельно. */
    printf("Hello World from thread %d of %d\n",
           data->thread_id, data->total_threads);

    /* Завершаем поток. Возвращаем NULL, т.к. ничего не нужно передавать обратно. */
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    pthread_t threads[NUM_THREADS];       /* Массив идентификаторов потоков */
    thread_data_t thread_data[NUM_THREADS]; /* Массив данных для каждого потока */
    int i, rc;

    for (i = 0; i < NUM_THREADS; i++) {
        /* Заполняем структуру данных для потока */
        thread_data[i].thread_id = i;
        thread_data[i].total_threads = NUM_THREADS;

        /* Создаём поток.
         * Аргументы:
         *   &threads[i]    — указатель на идентификатор создаваемого потока
         *   NULL           — атрибуты потока (по умолчанию)
         *   hello_func     — функция, которую будет выполнять поток
         *   &thread_data[i] — аргумент, передаваемый в функцию потока */
        rc = pthread_create(&threads[i], NULL, hello_func, (void *)&thread_data[i]);
        if (rc) {
            fprintf(stderr, "Ошибка создания потока %d: код %d\n", i, rc);
            exit(1);
        }
    }

    /* Ожидаем завершения всех потоков.
     * pthread_join блокирует вызывающий поток до завершения указанного потока. */
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Все потоки завершили работу.\n");
    return 0;
}