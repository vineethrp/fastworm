#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>

#include "log.h"
#include "argparser.h"
#include "segmenter.h"
#include "segmentation.h"

bool debug_run = false;
long num_tasks = 0;

static int filepath(int padding, int num, char *prefix, char *ext, char *path);
int write_output(char *filepath, report_t *reports, int count);
void * task_segmenter(void *data);

int
main(int argc, char **argv)
{
  int i = 0;
  pthread_t thrs[THREADS_MAX];
  int frames_per_thread;
  char file_path[PATH_MAX];
  segment_task_t task = { 0 }, main_task = { 0 };
  segment_task_t thr_tasks[THREADS_MAX];

  if (parse_arguments(argc, argv, &task) < 0) {
    fprintf(stderr, "Failed to parse arguments\n");
    exit(1);
  }

  if (log_init(task.verbosity, 0, task.logfile) < 0) {
    exit (1);
  }

  if (task.count < num_tasks)
    num_tasks = task.count;

  frames_per_thread = task.count / num_tasks;

  task.start = 0;
  task.reports = (report_t *) calloc(task.count, sizeof(report_t));
  if (task.reports == NULL) {
    LOG_ERR("Failed to allocate reports!");
    return -1;
  }

  LOG_INFO("Starting Segmenter");
  /*
   * start num_tasks - 1 threads (this main thread also does job!)
   */
  for (i = 0; i < num_tasks - 1; i++) {
    thr_tasks[i] = task;
    thr_tasks[i].count = frames_per_thread;
    thr_tasks[i].start = i * frames_per_thread;
    if (pthread_create(&thrs[i], NULL, task_segmenter, (void *)&thr_tasks[i]) < 0) {
      LOG_ERR("Failed to create thread %d!", i);
      return -1;
    }
  }

  /*
   * Rest of the work in Main thread!
   */
  main_task = task;
  main_task.count = task.count - i + 1;
  main_task.start = i * frames_per_thread;
  task_segmenter(&main_task);

  /*
   * Wait for all worker threads!
   */
  for (i = 0; i < num_tasks - 1; i++) {
    int *ret_val;
    pthread_join(thrs[i], (void *)&ret_val);
    if ((long long)ret_val != 0) {
      LOG_ERR("Thread %d failed!", i);
    }
  }

  /*
   * Print the results returned by all workers.
   */
  sprintf(file_path, "%s/%s", task.output_dir, task.outfile);
  write_output(file_path, task.reports, task.count);

  log_fini();
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
    task->reports[i].frame_id = i;
    task->reports[i].centroid_x = segdata.centroid_x;
    task->reports[i].centroid_y = segdata.centroid_y;
    task->reports[i].area = segdata.area;
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
