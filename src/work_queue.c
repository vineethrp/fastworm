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
pop_work(wq_t *wq, work_t *w)
{
  if (wq == NULL || w == NULL) {
    LOG_ERR("Invalid arguements to get_work");
    return -1;
  }

  pthread_mutex_lock(&wq->q_mutex);
  while (wq->sz == 0) {
    pthread_cond_wait(&wq->q_cond, &wq->q_mutex);
  }

  *w = wq->q[wq->tail % wq->cap];
  wq->tail++;
  wq->sz--;

  /*
   * Signal the producer if the queue is empty.
   */
  if (wq->sz == 0)
    pthread_cond_broadcast(&wq->q_cond);

  pthread_mutex_unlock(&wq->q_mutex);

  return 0;
}

int
push_work(wq_t *wq, work_t w)
{
  if (wq == NULL) {
    LOG_ERR("Invalid arguements to put_work");
    return -1;
  }

  pthread_mutex_lock(&wq->q_mutex);
  while (wq->sz == wq->cap) {
    pthread_cond_wait(&wq->q_cond, &wq->q_mutex);
  }

  wq->q[wq->head % wq->cap] = w;
  wq->head++;
  wq->sz++;

  if (wq->sz == 1)
    pthread_cond_broadcast(&wq->q_cond);

  pthread_mutex_unlock(&wq->q_mutex);
  return 0;
}
