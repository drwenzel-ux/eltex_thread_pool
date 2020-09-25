#include "pool.h"

volatile sig_atomic_t stop;

void handler(int sig) { stop = 1; }

void routine(int fd) {
  char buffer[64];

  memset(buffer, 0, 64);
  snprintf(buffer, 64, "hello, from %ld\n", pthread_self());

  write(fd, buffer, 64);
  sleep(rand() % 10);
  write(fd, buffer, 64);

  close(fd);
}

int main(int argc, char const *argv[]) {
  int server_fd;
  int client_fd;
  struct pool *tp;

  set_handler(handler, SIGINT, 0);

  server_fd = inet_listen("127.0.0.1", 3000, 5);
  tp = pool_create(10);

  while (1) {
    client_fd = inet_accept(server_fd);

    if (stop)
      break;

    if (client_fd == -1) {
      perror("accept failed!");
      continue;
    }

    add_task(tp, routine, client_fd);
  }

  pool_destroy(tp);
  close(server_fd);

  return 0;
}
