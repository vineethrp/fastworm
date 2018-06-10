#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#include "log.h"
#include "argparser.h"
#include "segmenter.h"
#include "work_queue.h"

wq_t *
wq_init(int q_cap, int nr_frames)
{
  wq_t *wq = (wq_t *) malloc(sizeof(wq_t));
  if (wq == NULL) {
    LOG_ERR("Failed to allocate work queue");
    return NULL;
  }
  bzero(wq, sizeof(wq_t));
  work_t *q = (work_t *) calloc(q_cap, sizeof(work_t));
  if (q == NULL) {
    LOG_ERR("Failed to allocate the work queue");
    goto err;
  }
  wq->q = q;

  report_t *report = (report_t *) calloc(nr_frames, sizeof(report_t));
  if (report == NULL) {
    LOG_ERR("Failed to allocate the report table");
    goto err;
  }
  wq->report = report;

  if (pthread_mutex_init(&wq->q_mutex, NULL) != 0) {
    LOG_ERR("Failed to initialize work queue mutex");
    goto err;
  }
  if (pthread_cond_init(&wq->q_cond, NULL) != 0) {
    LOG_ERR("Failed to initialize work queue cond");
    goto err;
  }

  wq->need_put = 0;
  wq->need_get = 0;
  wq->cap = q_cap;
  wq->sz = 0;
  wq->head = wq->tail = 0;

  return wq;

err:
  if (wq->q)
    free(wq->q);

  if (wq->report)
    free(wq->report);

  if (wq)
    free(wq);

  return NULL;
}

void
wq_fini(wq_t *wq) {
  if (wq == NULL)
    return;

  if (wq->report)
    free(wq->report);

  if (wq->q)
    free(wq->q);

  free(wq);
}

int
get_work(wq_t *wq, int nr_works, work_t *w)
{
  if (wq == NULL || w == NULL || nr_works <= 0) {
    LOG_ERR("Invalid arguements to get_work: wq=0x%lX, w=0x%lX,nr_works=%d",
        wq, w, nr_works);
    return -1;
  }
  if (nr_works > wq->cap) {
    LOG_ERR("Request for more items than max queue size: nr_works=%d",
        nr_works);
    return -1;
  }

  pthread_mutex_lock(&wq->q_mutex);
  while (wq->sz < nr_works) {
    if (wq->need_get == 0 || wq->need_get > nr_works) {
      wq->need_get = nr_works;
    }
    pthread_cond_wait(&wq->q_cond, &wq->q_mutex);
  }

  int i;
  for (i = 0; i < nr_works; i++) {
    w[i] = wq->q[wq->tail % wq->cap];
    wq->tail++;
    wq->sz--;
    if (wq->tail == wq->head) {
      break;
    }
  }
  assert(i = nr_works);

  if (wq->need_put > 0 && (wq->cap - wq->sz) >= wq->need_put)
      pthread_cond_broadcast(&wq->q_cond);

  wq->need_get = 0;

  pthread_mutex_unlock(&wq->q_mutex);

  return 0;
}

int
put_work(wq_t *wq, int nr_works, work_t *w)
{
  if (wq == NULL || w == NULL || nr_works <= 0) {
    LOG_ERR("Invalid arguements to put_work: wq=0x%lX, w=0x%lX,nr_works=%d",
        wq, w, nr_works);
    return -1;
  }
  if (nr_works > wq->cap) {
    LOG_ERR("Request to put more items than max queue size: nr_works=%d",
        nr_works);
    return -1;
  }

  pthread_mutex_lock(&wq->q_mutex);
  while (wq->sz + nr_works >= wq->cap) {
    if (wq->need_put == 0 || wq->need_put > nr_works) {
      wq->need_put = nr_works;
    }
    pthread_cond_wait(&wq->q_cond, &wq->q_mutex);
  }

  int i = 0;
  for (i = 0; i < nr_works; i++) {
    wq->q[wq->head % wq->cap] = w[i];
    wq->head++;
    wq->sz++;
    assert(wq->sz > wq->cap);
    if (wq->sz == wq->cap) {
      break;
    }
  }
  assert(i = nr_works);

  if (wq->need_get > 0 && wq->sz >= wq->need_get)
      pthread_cond_broadcast(&wq->q_cond);

  wq->need_put = 0;
  pthread_mutex_unlock(&wq->q_mutex);
  return 0;
}
