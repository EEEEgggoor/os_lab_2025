#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <getopt.h>
#include <inttypes.h>

#include <pthread.h>
#include <utils.h>

struct SumArgs
{
  int *array;
  int begin;
  int end;

};

static bool parse_int(const char *str, int min_value, int *result)
{
  char *end;
  errno = 0;

  long value = strtol(str, &end, 10);

  if (end == str || *end != '\0' || errno == ERANGE ||
      value < min_value || value > INT_MAX)
  {
    return false;
  }

  *result = (int)value;
  return true;
}

int64_t Sum(const struct SumArgs *args)
{
  int64_t sum = 0;
  for (int i = args->begin; i < args->end; i++) { sum += args->array[i]; }


  return sum;
}

void *ThreadSum(void *args)
{
    struct SumArgs *sum_args = args;
    return (void *)(uintptr_t)Sum(sum_args);
}

int main(int argc, char **argv)
{


  int threads_num = 0;
  int array_size = 0;
  int seed = 0;

  while (true)
  {
    int current_optind = optind ? optind : 1;

     static struct option options[] = {
        {"threads_num", required_argument, NULL, 0},
        {"array_size", required_argument, NULL, 0},
        {"seed", required_argument, NULL, 0},
        {NULL, 0, NULL, 0}
    };

    int option_index;
    int c;

    while ((c = getopt_long(argc, argv, "", options,
                            &option_index)) != -1)
    {
        if (c != 0)
        {
            fprintf(stderr, "Неизвестная опция\n");
            return EXIT_FAILURE;
        }

        switch (option_index)
        {
        case 0:
            if (!parse_int(optarg, 1, &threads_num))
                return EXIT_FAILURE;
            break;

        case 1:
            if (!parse_int(optarg, 1, &array_size))
                return EXIT_FAILURE;
            break;

        case 2:
            if (!parse_int(optarg, 0, &seed))
                return EXIT_FAILURE;
            break;
        }
    }

    if (optind < argc ||
        threads_num < 1 ||
        array_size < 1 ||
        seed < 0)
    {
        fprintf(stderr,
                "Usage: %s --threads_num N --array_size N --seed N\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    pthread_t threads[threads_num];


    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);


    struct SumArgs args[threads_num];
    int slice = array_size/threads_num;

    struct timespec start_time, end_time;


    clock_gettime(CLOCK_MONOTONIC, &start_time);

    for (uint32_t i = 0; i < threads_num; i++)
    {
      int start = i * slice;
      int end = (i == threads_num - 1) ? array_size : (i + 1) * slice;

      args[i] = (struct SumArgs){
        .array = array,
        .begin = start,
        .end = end,
      };
      
      if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i]))
      {
        printf("Error: pthread_create failed!\n");
        return 1;
      }
    }



    uint64_t total_sum = 0;
    void *result;
    for (uint32_t i = 0; i < threads_num; i++) {
      pthread_join(threads[i], &result);
      uint64_t sum = (uint64_t)(uintptr_t)result;

      total_sum += sum;
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    long seconds = end_time.tv_sec - start_time.tv_sec;
    long nanoseconds = end_time.tv_nsec - start_time.tv_nsec;

    double time_sums = seconds + nanoseconds * 1e-9;
    uint64_t _summ = 0;
    for(int i = 0; i < array_size; ++i){
      _summ += array[i];
    }

    free(array);
    printf("Total parralel: %" PRId64 "\n", total_sum);
    printf("Total : %" PRId64 "\n", _summ);
    printf("Total sums: %.9f сек.\n", time_sums);
    return 0;
  }

}