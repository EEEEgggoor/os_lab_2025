#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

static bool parse_int(const char *str, int min_value, int *result) {
  char *end;
  errno = 0;

  long value = strtol(str, &end, 10);

  if (end == str || *end != '\0' || errno == ERANGE ||
      value < min_value || value > INT_MAX) {
    return false;
  }

  *result = (int)value;
  return true;
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            if (!parse_int(optarg, 0, &seed)) {
              fprintf(stderr, "Ошибка: seed должен быть целым числом от 0 до %d\n",
                      INT_MAX);
              return 1;
            }
            break;

          case 1:
            if (!parse_int(optarg, 1, &array_size)) {
              fprintf(stderr, "Ошибка: array_size должен быть целым числом от 1 до %d\n",
                      INT_MAX);
              return 1;
            }
            break;

          case 2:
            if (!parse_int(optarg, 1, &pnum)) {
              fprintf(stderr, "Ошибка: pnum должен быть целым числом от 1 до %d\n",
                      INT_MAX);
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);
  int fd;
  if(with_files){
    fd = open("file_MIN_MAX.txt", O_RDWR | O_CREAT | O_APPEND | O_TRUNC, 0666);
  }


  int pipefds[pnum][2];


  for (int i = 0; i < pnum; i++) {

    if(pipe(pipefds[i]) == -1){
      perror("fork");
      exit(EXIT_FAILURE);
    }

    pid_t child_pid = fork();

    int slice = array_size/pnum;

    if (child_pid >= 0) {
      if (child_pid == 0) {
        int start = i * slice;
        int end = (i == pnum - 1) ? array_size : (i + 1) * slice;
        struct MinMax min_max = GetMinMax(array, start, end);

        int in_file[2] = {min_max.min, min_max.max};

        if (with_files) {
          write(fd, in_file, sizeof(in_file));
        }
        else {
          close(pipefds[i][0]);
          
          write(pipefds[i][1], in_file, sizeof(in_file));

          close(pipefds[i][1]);

        }
        return 0;
      } else {
        active_child_processes += 1;
            
        if (!with_files) {
            close(pipefds[i][1]); 
        }

      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  if (with_files) {
    lseek(fd, 0, SEEK_SET);
  }

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    int out_file[2];

    if (with_files) {
      read(fd, out_file, sizeof(out_file));

      min = out_file[0];
      max = out_file[1];
    } else {
      read(pipefds[i][0], out_file, sizeof(out_file));
        
      close(pipefds[i][0]);
    }

    min = out_file[0];
    max = out_file[1];

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  close(fd);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}