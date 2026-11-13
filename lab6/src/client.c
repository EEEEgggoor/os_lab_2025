#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

struct Server {
  char ip[255];
  int port;
};

struct ClientTask {
  struct Server server;
  struct FactorialArgs args;
  uint64_t result;
};

void *ThreadServer(void *arg) {
  struct ClientTask *task = (struct ClientTask *)arg;

  struct hostent *hostname = gethostbyname(task->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", task->server.ip);
    exit(1);
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(task->server.port);
  server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  //Создаем TCP-сокет
  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    exit(1);
  }

  //Подключаемся к серверу
  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Connection failed\n");
    exit(1);
  }

  uint64_t to_send[3] = {task->args.begin, task->args.end, task->args.mod};
  if (send(sck, to_send, sizeof(to_send), 0) < 0) {
    fprintf(stderr, "Send failed\n");
    exit(1);
  }

  // 4. Получаем ответ от сервера напрямую в переменную result
  if (recv(sck, &task->result, sizeof(task->result), 0) < 0) {
    fprintf(stderr, "Receive failed\n");
    exit(1);
  }

  // 5. Закрываем соединение
  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'};

  while (true) {
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        break;
      case 2:
        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  FILE *f = fopen(servers, "r");
  if (f == NULL) {
    fprintf(stderr, "Can not open %s\n", servers);
    return 1;
  }

  unsigned int servers_num = 0;
  struct Server *to = malloc(sizeof(struct Server) * 100);
  while (fscanf(f, " %254[^:]:%d", to[servers_num].ip, &to[servers_num].port) == 2) {
    servers_num++;
  }
  fclose(f);

  // Распараллеливаем работу между серверами с помощью потоков
  pthread_t threads[servers_num];
  struct ClientTask tasks[servers_num];
  uint64_t part = k / servers_num;

  for (unsigned int i = 0; i < servers_num; i++) {
    tasks[i].server = to[i];
    tasks[i].args.begin = 1 + i * part;
    tasks[i].args.end = (i == servers_num - 1) ? k : (i + 1) * part;
    tasks[i].args.mod = mod;

    pthread_create(&threads[i], NULL, ThreadServer, (void *)&tasks[i]);
  }

  // Ждем завершения всех серверов и перемножаем их результаты
  uint64_t answer = 1;
  for (unsigned int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    answer = MultModulo(answer, tasks[i].result, mod);
  }

  printf("answer: %llu\n", (unsigned long long)answer);
  free(to);

  return 0;
}
