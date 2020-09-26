#include "pool.h"

volatile sig_atomic_t stop;

void handler(int sig) { stop = 1; }

void udp_task(void *arg) {
  struct connection *c;
  char buffer[64];

  c = (struct connection *)arg;
  memset(buffer, 0, 64);
  snprintf(buffer, 64, "hello, from %ld\n", pthread_self());

  sendto(c->fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&c->sa,
         sizeof(c->sa));
  sleep(rand() % 10);
  sendto(c->fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&c->sa,
         sizeof(c->sa));

  mem_free(c);
}

int main(int argc, char const *argv[]) {
  int ret;
  int server_fd;
  char buffer[10];

  struct sockaddr_in sa;
  socklen_t len = sizeof(sa);

  struct pool *p;
  struct connection *c;

  set_handler(handler, SIGINT, 0);

  server_fd = inet_bind("127.0.0.1", 3000, SOCK_DGRAM);
  p = pool_create(5);

  while (1) {
    ret = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sa,
                   &len);

    if (stop)
      break;

    if (ret == -1) {
      perror("recvfrom failed");
      break;
    }

    c = mem_calloc(1, sizeof(*c));
    c->fd = server_fd;
    c->sa = sa;

    add_task(p, udp_task, c);
  }

  pool_destroy(p);
  close(server_fd);

  return 0;
}
