/*
 * Последовательный доступ к глобальной переменной с использованием mutex.
 * Потоки по порядку (определяемому номером создания) получают доступ
 * к общей переменной, инкрементируют её и выводят результат.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 4

/* Глобальная переменная, к которой обращаются все потоки */
int shared_value = 0;

/* Mutex для защиты критической секции */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Переменная условия для обеспечения порядка доступа */
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* Номер потока, который сейчас должен получить доступ */
int current_turn = 0;

/* Функция потока */
void *sequential_access(void *arg)
{
    int my_id = *(int *)arg;

    /* Захватываем mutex */
    pthread_mutex_lock(&mutex);

    /* Ждём, пока не наступит наша очередь.
     * pthread_cond_wait атомарно освобождает mutex и засыпает.
     * При пробуждении mutex снова захватывается. */
    while (current_turn != my_id) {
        pthread_cond_wait(&cond, &mutex);
    }

    /* Критическая секция: инкрементируем общую переменную */
    shared_value++;
    printf("[Поток %d] Значение переменной: %d\n", my_id, shared_value);

    /* Передаём очередь следующему потоку */
    current_turn++;

    /* Будим все ожидающие потоки, чтобы следующий по очереди мог продолжить */
    pthread_cond_broadcast(&cond);

    /* Освобождаем mutex */
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int i;

    /* Создаём потоки */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, sequential_access, (void *)&thread_ids[i]);
    }

    /* Ожидаем завершения всех потоков */
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Итоговое значение: %d\n", shared_value);

    /* Уничтожаем примитивы синхронизации */
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}