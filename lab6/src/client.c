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


struct Task {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};

void *WorkWithServer(void *arg) {
  struct Task *t = (struct Task *)arg;

  struct hostent *hostname = gethostbyname(t->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", t->server.ip);
    exit(1);
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(t->server.port);
  server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    exit(1);
  }

  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Connection failed\n");
    exit(1);
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &t->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &t->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &t->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed\n");
    exit(1);
  }

  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Recieve failed\n");
    exit(1);
  }

  memcpy(&t->result, response, sizeof(uint64_t));
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
        strncpy(servers, optarg, sizeof(servers) - 1);
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

  // читаем сервера из файла (формат строки: ip:port)
  FILE *f = fopen(servers, "r");
  if (f == NULL) {
    fprintf(stderr, "Can not open %s\n", servers);
    return 1;
  }

  unsigned int servers_num = 0;
  struct Server *to = malloc(sizeof(struct Server) * 100);
  while (servers_num < 100 &&
         fscanf(f, " %254[^:]:%d", to[servers_num].ip,
                &to[servers_num].port) == 2)
    servers_num++;
  fclose(f);

  if (servers_num == 0) {
    fprintf(stderr, "No servers in file\n");
    return 1;
  }

  // по одному потоку на сервер, чтобы сервера считали одновременно
  pthread_t threads[servers_num];
  struct Task tasks[servers_num];
  uint64_t part = k / servers_num;

  for (unsigned int i = 0; i < servers_num; i++) {
    tasks[i].server = to[i];
    tasks[i].begin = 1 + i * part;
    tasks[i].end = (i == servers_num - 1) ? k : (i + 1) * part;
    tasks[i].mod = mod;
    pthread_create(&threads[i], NULL, WorkWithServer, &tasks[i]);
  }

  // объединяем результаты
  uint64_t answer = 1;
  for (unsigned int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    answer = MultModulo(answer, tasks[i].result, mod);
  }

  printf("answer: %llu\n", (unsigned long long)answer);
  free(to);

  return 0;
}
