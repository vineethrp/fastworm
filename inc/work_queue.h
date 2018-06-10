#pragma once

typedef struct work_s {
  int frame;
  int centroid_x;
  int centroid_y;
  int area;
  char path[PATH_MAX];
} work_t;

typedef struct wq_s {
  work_t *q;
  report_t *report;
  int cap;
  int sz;
  int head;
  int tail;
  pthread_mutex_t q_mutex;
  pthread_cond_t q_cond;
  int need_put;
  int need_get;
} wq_t;

wq_t *wq_init();
void wq_fini(wq_t *);
int get_work(wq_t *wq, int nr_works, work_t *w);
int put_work(wq_t *wq, int nr_works, work_t *w);
