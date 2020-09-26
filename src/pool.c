#include "pool.h"

static struct task *task_create(void (*func)(void *), void *arg) {
  struct task *task;

  if (func == NULL)
    return NULL;

  task = mem_calloc(1, sizeof(*task));
  task->func = func;
  task->arg = arg;
  task->next = NULL;
  return task;
}

static void task_destroy(struct task *task) {
  if (task == NULL)
    return;

  mem_free(task);
}

static struct task *task_get(struct pool *p) {
  struct task *task;

  if (p == NULL)
    return NULL;

  task = p->first;
  if (task == NULL)
    return NULL;

  if (task->next == NULL) {
    p->first = NULL;
    p->last = NULL;
  } else {
    p->first = task->next;
  }

  return task;
}

static void *pool_worker(void *arg) {
  printf("thread #%ld start\n", pthread_self());

  struct pool *p = arg;
  struct task *task;

  while (1) {
    pthread_mutex_lock(&p->task_mutex);
    if (p->stop)
      break;

    if (p->first == NULL)
      pthread_cond_wait(&p->task_cond, &p->task_mutex);

    task = task_get(p);
    p->working_cnt++;
    pthread_mutex_unlock(&p->task_mutex);

    if (task != NULL) {
      task->func(task->arg);
      task_destroy(task);
    }

    pthread_mutex_lock(&p->task_mutex);
    p->working_cnt--;

    if (!p->stop && p->working_cnt == 0 && p->first == NULL)
      pthread_cond_signal(&p->working_cond);
    pthread_mutex_unlock(&p->task_mutex);
  }

  p->thread_cnt--;
  printf("thread #%ld end\n", pthread_self());

  if (p->thread_cnt == 0)
    pthread_cond_signal(&p->working_cond);
  pthread_mutex_unlock(&p->task_mutex);
  return NULL;
}

struct pool *pool_create(size_t num) {
  struct pool *p;
  pthread_t thread;
  size_t i;

  if (num == 0)
    num = 2;

  p = mem_calloc(1, sizeof(*p));
  p->thread_cnt = num;

  pthread_mutex_init(&p->task_mutex, NULL);
  pthread_cond_init(&p->task_cond, NULL);
  pthread_cond_init(&p->working_cond, NULL);

  p->first = NULL;
  p->last = NULL;

  for (i = 0; i < num; i++) {
    pthread_create(&thread, NULL, pool_worker, p);
    pthread_detach(thread);
  }

  return p;
}

void pool_destroy(struct pool *p) {
  struct task *task;
  struct task *task2;

  if (p == NULL)
    return;

  pthread_mutex_lock(&p->task_mutex);

  task = p->first;
  while (task != NULL) {
    task2 = task->next;
    task_destroy(task);
    task = task2;
  }

  p->stop = 1;

  pthread_cond_broadcast(&p->task_cond);
  pthread_mutex_unlock(&p->task_mutex);

  pool_wait(p);

  pthread_mutex_destroy(&p->task_mutex);
  pthread_cond_destroy(&p->task_cond);
  pthread_cond_destroy(&p->working_cond);

  mem_free(p);
}

int add_task(struct pool *p, void (*f)(void *), void *arg) {
  struct task *task;

  if (p == NULL)
    return 1;

  task = task_create(f, arg);
  if (task == NULL)
    return 1;

  pthread_mutex_lock(&p->task_mutex);

  if (p->first == NULL) {
    p->first = task;
    p->last = p->first;
  } else {
    p->last->next = task;
    p->last = task;
  }

  pthread_cond_broadcast(&p->task_cond);
  pthread_mutex_unlock(&p->task_mutex);

  return 0;
}

void pool_wait(struct pool *p) {
  if (p == NULL)
    return;

  pthread_mutex_lock(&p->task_mutex);

  while (1) {
    if ((!p->stop && p->working_cnt != 0) || (p->stop && p->thread_cnt != 0))
      pthread_cond_wait(&p->working_cond, &p->task_mutex);
    else
      break;
  }

  pthread_mutex_unlock(&p->task_mutex);
}