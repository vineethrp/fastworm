#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "log.h"
#include "argparser.h"
#include "segmenter.h"
#include "segmentation.h"

bool debug_run = false;

static int filepath(int padding, int num, char *prefix, char *ext, char *path);
int write_output(char *filepath, report_t *reports, int count);
int task_segmenter(segment_task_t *task);

int
main(int argc, char **argv)
{
  segment_task_t task = { 0 };

  if (parse_arguments(argc, argv, &task) < 0) {
    fprintf(stderr, "Failed to parse arguments\n");
    exit(1);
  }

  if (log_init(task.verbosity, 0, task.logfile) < 0) {
    exit (1);
  }

  LOG_INFO("Starting Segmenter");
  task.start = 0;
  task_segmenter(&task);

  log_fini();
}

int
task_segmenter(segment_task_t *task)
{
  int ret = 0;
  int i;
  char file_path[PATH_MAX];
  segdata_t segdata = { 0 };
  int last = task->start + task->count;

  report_t *reports = (report_t *) calloc(task->count, sizeof(report_t));
  if (reports == NULL) {
    fprintf(stderr, "Failed to allocate reports\n");
    return -1;
  }

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
    reports[i - task->start].frame_id = i;
    reports[i - task->start].centroid_x = segdata.centroid_x;
    reports[i - task->start].centroid_y = segdata.centroid_y;
    reports[i - task->start].area = segdata.area;
    ret = 0;
  }

  sprintf(file_path, "%s/%s", task->output_dir, task->outfile);
  write_output(file_path, reports, task->count);
  segdata_fini(&segdata);
  return ret;
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
