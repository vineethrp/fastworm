#pragma once

#define WORK_QUEUE_DEFAULT_CAP 64

typedef struct work_s {
  int frame;
  int padding;
  int centroid_x;
  int centroid_y;
  int area;
  char path[PATH_MAX];
} work_t;

typedef struct wq_s {
  work_t *q;
  int cap;
  int sz;
  int head;
  int tail;
  bool done;
  pthread_mutex_t q_mutex;
  pthread_cond_t q_cond;
} wq_t;

wq_t *wq_init(int q_cap);
void wq_fini(wq_t *);
int wq_pop_work(wq_t *wq, work_t *w);
int wq_push_work(wq_t *wq, work_t w);
void wq_mark_done(wq_t *wq);
