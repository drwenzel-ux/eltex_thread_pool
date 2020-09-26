#include "pool.h"

volatile sig_atomic_t stop;

void handler(int sig) { stop = 1; }

void tcp_task(void *arg) {
  struct connection *c;
  char buffer[64];

  c = (struct connection *)arg;
  memset(buffer, 0, 64);
  snprintf(buffer, 64, "hello, from %ld\n", pthread_self());

  write(c->fd, buffer, 64);
  sleep(rand() % 10);
  write(c->fd, buffer, 64);

  close(c->fd);
  mem_free(c);
}

int main(int argc, char const *argv[]) {
  int server_fd;
  int client_fd;
  struct connection *c;
  struct pool *p;

  set_handler(handler, SIGINT, 0);

  server_fd = inet_listen("127.0.0.1", 3000, 5);
  p = pool_create(2);

  while (1) {
    client_fd = inet_accept(server_fd);

    if (stop)
      break;

    if (client_fd == -1) {
      perror("accept failed!");
      continue;
    }

    c = calloc(1, sizeof(*c));
    c->fd = client_fd;

    add_task(p, tcp_task, c);
  }

  pool_destroy(p);
  close(server_fd);

  return 0;
}
