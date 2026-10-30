#define _XOPEN_SOURCE 700

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;

int p = 1;

void inc(int* n) { ++(*n); }

void *worker1(void *arg) {
    (void)arg;

    pthread_mutex_lock(&mutex1);
    puts("Поток 1 захватил mutex1");
    sleep(1);
    inc(&p);

    puts("Поток 1 ждёт mutex2");
    pthread_mutex_lock(&mutex2);

    /* Эти строки не выполнятся из-за deadlock. */
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

void *worker2(void *arg) {
    (void)arg;

    pthread_mutex_lock(&mutex2);
    puts("Поток 2 захватил mutex2");
    sleep(1);
    inc(&p);

    puts("Поток 2 ждёт mutex1");
    pthread_mutex_lock(&mutex1);

    /* Эти строки не выполнятся из-за deadlock. */
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

void check(int error) {
    if (error != 0) {
        fprintf(stderr, "Ошибка pthread: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    pthread_t thread1, thread2;

    setbuf(stdout, NULL);

    check(pthread_barrier_init(&barrier, NULL, 2));

    check(pthread_create(&thread1, NULL, worker1, NULL));
    check(pthread_create(&thread2, NULL, worker2, NULL));

    check(pthread_join(thread1, NULL));
    check(pthread_join(thread2, NULL));

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    return 0;
}