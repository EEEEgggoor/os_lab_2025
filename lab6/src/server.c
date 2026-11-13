#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;
  for (uint64_t i = args->begin; i <= args->end; i++)
    ans = MultModulo(ans, i, args->mod);
  return ans;
}

void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  return (void *)(uint64_t *)Factorial(fargs);
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  while (true) {
    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        port = atoi(optarg);
        break;
      case 1:
        tnum = atoi(optarg);
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Unknown argument\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket!\n");
    return 1;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  //Привязываем сокет к порту
  if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Can not bind to socket!\n");
    return 1;
  }

  //Переводим сокет в режим ожидания подключений
  if (listen(server_fd, 128) < 0) {
    fprintf(stderr, "Could not listen on socket\n");
    return 1;
  }

  printf("Server listening at %d\n", port);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
    if (client_fd < 0) {
      fprintf(stderr, "Could not establish new connection\n");
      continue;
    }

    while (true) {

      uint64_t from_client[3];
      int read = recv(client_fd, from_client, sizeof(from_client), 0);

      if (!read)
        break;
      if (read < 0) {
        fprintf(stderr, "Client read failed\n");
        break;
      }
      if (read < (int)sizeof(from_client)) {
        fprintf(stderr, "Client send wrong data format\n");
        break;
      }

      uint64_t begin = from_client[0];
      uint64_t end = from_client[1];
      uint64_t mod = from_client[2];

      printf("Receive: %llu %llu %llu\n", (unsigned long long)begin,
             (unsigned long long)end, (unsigned long long)mod);

      pthread_t threads[tnum];
      struct FactorialArgs args[tnum];
      uint64_t part = (end - begin + 1) / tnum;

      for (int i = 0; i < tnum; i++) {
        args[i].begin = begin + i * part;
        args[i].end = (i == tnum - 1) ? end : args[i].begin + part - 1;
        args[i].mod = mod;

        if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
          printf("Error: pthread_create failed!\n");
          return 1;
        }
      }

      uint64_t total = 1;
      for (int i = 0; i < tnum; i++) {
        uint64_t result = 0;
        pthread_join(threads[i], (void **)&result);
        total = MultModulo(total, result, mod);
      }

      printf("Total: %llu\n", (unsigned long long)total);

      // Отправляем посчитанный результат
      if (send(client_fd, &total, sizeof(total), 0) < 0) {
        fprintf(stderr, "Can't send data to client\n");
        break;
      }
    }


    close(client_fd);
  }

  close(server_fd);
  return 0;
}
