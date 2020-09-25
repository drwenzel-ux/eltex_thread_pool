#ifndef __POOL_H__
#define __POOL_H__

#include "inet_socket.h"
#include <pthread.h>

struct task {
  void (*func)(int);
  int arg;
  struct task *next;
};

struct pool {
  struct task *first;
  struct task *last;
  pthread_mutex_t task_mutex;
  pthread_cond_t task_cond;
  pthread_cond_t working_cond;
  size_t working_cnt;
  size_t thread_cnt;
  int stop;
};

struct pool *pool_create(size_t num);
void pool_destroy(struct pool *tp);
int add_task(struct pool *tp, void (*f)(int), int arg);
void pool_wait(struct pool *tp);

#endif