#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    long long begin;
    long long end;
} Range;

int mod;
long long result = 1;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


long long factorial_range(long long begin, long long end) {
    long long value = 1 % mod;

    for (long long i = begin; i <= end; i++) {
        value = value * i % mod;
    }

    return value;
}

void *worker(void *arg) {
    Range *range = arg;

    long long local = factorial_range(range->begin, range->end);

    pthread_mutex_lock(&mutex);
    result = result * local % mod;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char **argv) {
    int k = -1;
    int pnum = 0;

    struct option options[] = {
        {"pnum", required_argument, NULL, 'p'},
        {"mod",  required_argument, NULL, 'm'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "k:p:m:", options, NULL)) != -1) {
        if (opt != 'k' && opt != 'p' && opt != 'm')
            goto usage;


        char *end;
        long long value = strtoll(optarg, &end, 10);

        if (end == optarg || *end != '\0' ||
            value < 0 || value > INT_MAX)
            goto usage;

        switch (opt) {
        case 'k': k = (int)value; break;
        case 'p': pnum = (int)value; break;
        case 'm': mod = (int)value; break;
        }
    }

    if (k < 0 || pnum <= 0 || mod <= 0 || optind != argc)
        goto usage;

    result = 1 % mod;

    if (k == 0) {
        printf("Результат: %lld\n", result);
        return 0;
    }

    if (pnum > k)
        pnum = k;

    pthread_t *threads = calloc(pnum, sizeof(*threads));
    Range *ranges = calloc(pnum, sizeof(*ranges));

    if (threads == NULL || ranges == NULL) {
        fprintf(stderr, "Не удалось выделить память\n");
        free(threads);
        free(ranges);
        return 1;
    }

    for (int i = 0; i < pnum; i++) {
        ranges[i].begin = (long long)k * i / pnum + 1;
        ranges[i].end = (long long)k * (i + 1) / pnum;

        int error = pthread_create(&threads[i], NULL, worker, &ranges[i]);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            exit(1);
        }
    }

    for (int i = 0; i < pnum; i++) {
        int error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            exit(1);
        }
    }

    printf("Результат: %lld\n", result);

    free(threads);
    free(ranges);
    pthread_mutex_destroy(&mutex);
    return 0;

usage:
    fprintf(stderr, "Запуск: %s -k 10 --pnum=4 --mod=10\n", argv[0]);
    return 1;
}