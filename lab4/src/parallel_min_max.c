#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

volatile sig_atomic_t is_timeout = 0;

void alarm_handler(int sign) {
  (void)sign;
  is_timeout = 1;
}

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
  int timeout = -1;
  bool with_files = false;

  static struct option options[] = {
      {"seed", required_argument, 0, 0},
      {"array_size", required_argument, 0, 0},
      {"pnum", required_argument, 0, 0},
      {"by_files", no_argument, 0, 'f'},
      {"timeout", required_argument, 0, 0},
      {0, 0, 0, 0}};

  while (true) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            if (!parse_int(optarg, 0, &seed)) {
              fprintf(stderr, "Ошибка: seed должен быть целым числом от 0 до %d\n", INT_MAX);
              return 1;
            }
            break;
          case 1:
            if (!parse_int(optarg, 1, &array_size)) {
              fprintf(stderr, "Ошибка: array_size должен быть целым числом от 1 до %d\n", INT_MAX);
              return 1;
            }
            break;
          case 2:
            if (!parse_int(optarg, 1, &pnum)) {
              fprintf(stderr, "Ошибка: pnum должен быть целым числом от 1 до %d\n", INT_MAX);
              return 1;
            }
            break;
          case 4:
            if (!parse_int(optarg, 1, &timeout)) {
              fprintf(stderr, "Ошибка: timeout должен быть положительным целым числом\n");
              return 1;
            }
            break;
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        return 1;
      default:
        return 1;
    }
  }

  if (optind < argc || seed < 0 || array_size < 1 || pnum < 1) {
    fprintf(stderr, "Usage: %s --seed N --array_size N --pnum N [--timeout N] [--by_files]\n", argv[0]);
    return 1;
  }

  int *array = malloc((size_t)array_size * sizeof(*array));
  if (!array) {
    perror("malloc");
    return 1;
  }
  GenerateArray(array, array_size, seed);

  int fd = -1;
  if (with_files) {
    fd = open("file_MIN_MAX.txt", O_RDWR | O_CREAT | O_APPEND | O_TRUNC, 0666);
    if (fd == -1) {
      perror("open");
      free(array);
      return 1;
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int pipefds[pnum][2];
  pid_t children[pnum];
  int created = 0;
  bool error = false;

  // Первый цикл: создаём все процессы, пока НЕ ждём их завершения.
  for (int i = 0; i < pnum; i++) {
    if (!with_files && pipe(pipefds[i]) == -1) {
      perror("pipe");
      error = true;
      break;
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
      perror("fork");
      if (!with_files) {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
      }
      error = true;
      break;
    }

    if (child_pid == 0) {
      sleep(5); // Для проверки таймаута.
      int slice = array_size / pnum;
      int start = i * slice;
      int end = (i == pnum - 1) ? array_size : (i + 1) * slice;
      struct MinMax mm = GetMinMax(array, start, end);
      int result[2] = {mm.min, mm.max};

      if (!with_files) {
        close(pipefds[i][0]);
      }
      int output_fd = with_files ? fd : pipefds[i][1];
      ssize_t bytes = write(output_fd, result, sizeof(result));
      if (!with_files) close(pipefds[i][1]);
      _exit(bytes == (ssize_t)sizeof(result) ? 0 : 1);
    }

    children[created++] = child_pid;
    if (!with_files) close(pipefds[i][1]);
  }

  // Один общий таймер запускается ПОСЛЕ создания всех детей.
  if (!error && timeout > 0) {
    struct sigaction sa = {0};
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
      perror("sigaction");
      error = true;
    } else {
      alarm((unsigned int)timeout);
    }
  }

  // Второй цикл: ждём детей, которые уже запущены параллельно.
  for (int i = 0; i < created && !error && !is_timeout; i++) {
    int status;
    while (!is_timeout) {
      pid_t res = waitpid(children[i], &status, WNOHANG);
      if (res == children[i]) {
        children[i] = 0; // Успешно забрали статус; больше не надо ждать или убивать.
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
          fprintf(stderr, "Дочерний процесс завершился с ошибкой\n");
          error = true;
        }
        break;
      }
      if (res == -1) {
        if (errno == EINTR) continue;
        perror("waitpid");
        error = true;
        break;
      }
      struct timespec delay = {.tv_sec = 0, .tv_nsec = 10000000};
      nanosleep(&delay, NULL);
    }
  }
  alarm(0);

  bool all_completed = true;
  for (int i = 0; i < created; i++) {
    if (children[i] > 0) all_completed = false;
  }

  if ((is_timeout && !all_completed) || error) {
    if (is_timeout) fprintf(stderr, "Timeout: SIGKILL незавершённым дочерним процессам\n");
    // Сначала остановим всех оставшихся, потом соберём их статусы.
    for (int i = 0; i < created; i++) {
      if (children[i] <= 0) continue;
      int status;
      pid_t res = waitpid(children[i], &status, WNOHANG);
      if (res == children[i] || (res == -1 && errno == ECHILD)) {
        children[i] = 0;
      } else if (res == 0) {
        if (kill(children[i], SIGKILL) == -1) {
          if (errno != ESRCH) perror("kill");
        } else if (is_timeout) {
          fprintf(stderr, "SIGKILL -> PID %ld\n", (long)children[i]);
        }
      }
    }
    for (int i = 0; i < created; i++) {
      if (children[i] <= 0) continue;
      int status;
      pid_t res;
      do {
        res = waitpid(children[i], &status, 0);
      } while (res == -1 && errno == EINTR);
      if (res == -1 && errno != ECHILD) perror("waitpid");
    }
    if (!with_files) {
      for (int i = 0; i < created; i++) close(pipefds[i][0]);
    }
    if (with_files) close(fd);
    free(array);
    return 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  if (with_files && lseek(fd, 0, SEEK_SET) == -1) {
    perror("lseek");
    close(fd);
    free(array);
    return 1;
  }

  for (int i = 0; i < pnum; i++) {
    int out_file[2];
    int input_fd = with_files ? fd : pipefds[i][0];
    ssize_t bytes = read(input_fd, out_file, sizeof(out_file));
    if (!with_files) close(pipefds[i][0]);
    if (bytes != (ssize_t)sizeof(out_file)) {
      fprintf(stderr, "Failed to read child result\n");
      if (with_files) close(fd);
      free(array);
      return 1;
    }
    if (out_file[0] < min_max.min) min_max.min = out_file[0];
    if (out_file[1] > min_max.max) min_max.max = out_file[1];
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);
  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  if (with_files) close(fd);
  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}
