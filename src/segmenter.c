#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>

#include "segmenter.h"
#include "log.h"
#include "segmentation.h"

static int filepath(int padding, int num, char *prefix, char *ext, char *path);

int
dispatch_segmenter_tasks(segment_task_t *task)
{
  int i = 0;
  pthread_t thrs[THREADS_MAX];
  int frames_per_thread;
  int spilled_tasks, prev_start, prev_count;
  segment_task_t main_task = { 0 };
  segment_task_t thr_tasks[THREADS_MAX];

  if (task->count < task->num_tasks)
    task->num_tasks = task->count;

  frames_per_thread = task->count / task->num_tasks;

  task->reports = (report_t *) calloc(task->count, sizeof(report_t));
  if (task->reports == NULL) {
    LOG_ERR("Failed to allocate reports!");
    return -1;
  }

  /*
   * start num_tasks - 1 threads (this main thread also does job!)
   *
   * Work splitting is like this:
   * frame per thread = total frames / number of threads
   * Main thread takes first frame_per_thread
   * then each thread takes one slice of frame_per_thread.
   * If the number of frames is not equally divisible by number of threads,
   * each thread takes one extra until there are no more spilled over frames.
   * This way, one thread is not overloaded with all the spilled over frames.
   */
  prev_start = task->base;
  prev_count = frames_per_thread;
  spilled_tasks = task->count % frames_per_thread;
  for (i = 0; i < task->num_tasks - 1; i++) {
    thr_tasks[i] = *task;
    thr_tasks[i].start = prev_start + prev_count;
    thr_tasks[i].count = frames_per_thread;
    if (spilled_tasks > 0) {
      thr_tasks[i].count++;
      spilled_tasks--;
    }
    prev_start = thr_tasks[i].start;
    prev_count = thr_tasks[i].count;
    LOG_XX_DEBUG("Thread %d: start=%d, count=%d", i, thr_tasks[i].start, thr_tasks[i].count);
    if (pthread_create(&thrs[i], NULL, task_segmenter, (void *)&thr_tasks[i]) < 0) {
      LOG_ERR("Failed to create thread %d!", i);
      goto err;
    }
  }

  /*
   * Rest of the work in Main thread!
   */
  main_task = *task;
  main_task.count = frames_per_thread;
  main_task.start = task->base;
  LOG_XX_DEBUG("Main Thread: start=%d, count=%d", main_task.start, main_task.count);
  task_segmenter(&main_task);

  /*
   * Wait for all worker threads!
   */
  for (i = 0; i < task->num_tasks - 1; i++) {
    int *ret_val;
    pthread_join(thrs[i], (void *)&ret_val);
    if ((long long)ret_val != 0) {
      LOG_ERR("Thread %d failed!", i);
      goto err;
    }
  }
  return 0;

err:
  free(task->reports);
  return -1;
}

void *
task_segmenter(void *data)
{
  int ret = 0;
  int i, last;
  char file_path[PATH_MAX];
  segment_task_t *task = NULL;
  segdata_t segdata = { 0 };

  task = (segment_task_t *)data;
  last = task->start + task->count;

  for (i = task->start; i < last; i++) {
    int report_index = i - task->base;
    ret = -1;
    if (filepath(task->padding, i, task->input_dir, task->ext, file_path) < 0) {
      LOG_ERR("Failed to create filepath!");
      break;
    }
    if (i == task->start) {
      if (segdata_init(task, file_path, &segdata) < 0) {
        LOG_ERR("Failed to initialize segdata");
        break;
      }
    } else {
      if (segdata_reset(&segdata, file_path) < 0) {
        LOG_ERR("Failed to reset segdata");
        break;
      }
    }
    segdata_process(&segdata);
    task->reports[report_index].frame_id = i;
    task->reports[report_index].centroid_x = segdata.centroid_x;
    task->reports[report_index].centroid_y = segdata.centroid_y;
    task->reports[report_index].area = segdata.area;
    ret = 0;
  }

  segdata_fini(&segdata);
  return (void *)(unsigned long long)ret;
}

static int
filepath(int padding, int num, char *prefix, char *ext, char *path)
{
  char format[NAME_MAX];
  if (path == NULL)
    return -1;

  if (sprintf(format, "%s/%%0%dd.%s", prefix, padding, ext) <= 0) {
   return -1;
  }

  if (sprintf(path, format, num) <= 0) {
    return -1;
  }

  return 0;
}

int
write_output(char *filepath, report_t *reports, int count)
{
  int i;
  FILE *f = fopen(filepath, "w");
  if (f == NULL) {
    fprintf(stderr, "Failed to open output file\n");
    return -1;
  }
  for (i = 0; i < count; i++) {
    fprintf(f, "%d, %d, %d, %d\n",
        reports[i].frame_id, reports[i].centroid_y,
        reports[i].centroid_x, reports[i].area);
  }
  fclose(f);
  return 0;
}
