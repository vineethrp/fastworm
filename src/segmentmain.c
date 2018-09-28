#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <pthread.h>

#include "log.h"
#include "argparser.h"
#include "work_queue.h"
#include "segmenter.h"

#define MAX_IMG_WIDTH 3840
#define MAX_IMG_HEIGHT 2160
#define MAX_IMGBUF_SZ (MAX_IMG_WIDTH * MAX_IMG_HEIGHT)
static unsigned char blur_data[MAX_IMGBUF_SZ];
static unsigned char tmp_data[MAX_IMGBUF_SZ];
static bool threshold_data[MAX_IMGBUF_SZ];
static int integral_data[MAX_IMGBUF_SZ];

/*
 * Entry point for single frame segmenter.
 */

int
main(int argc, char *argv[])
{
  int ret = -1;
  char file_path[PATH_MAX + NAME_MAX];
  segdata_t segdata = { 0 };
  report_t report;
  segment_task_t task = { 0 };
  work_t w = { 0 };

  if (segment_task_init(argc, argv, &task, true) < 0) {
    fprintf(stderr, "Failed to initialize\n");
    return ret;
  }

  // create output directory
  errno = 0;
  if (mkdir(task.output_dir, 0750) < 0 && errno != EEXIST) {
    fprintf(stderr, "Failed to create the output directory!\n");
    goto out;
  }

  snprintf(file_path, PATH_MAX + NAME_MAX, "%s/%s", task.output_dir, task.logfile);
  if (log_init(task.verbosity, 0, file_path) < 0) {
    goto out;
  }

  w.centroid_x = task.centroid_x;
  w.centroid_y = task.centroid_y;
  w.frame = task.frame;
  w.padding = task.padding;
  if (segment_frame(&task, w, &segdata, true, &report,
                    blur_data, tmp_data, threshold_data, integral_data) == 0) {
    printf("%d %d %d %d\n",
        report.frame_id, report.centroid_x, report.centroid_y, report.area);
    ret = 0;
  } else {
    LOG_ERR("Failed to segment the frame");
  }

out:

  log_fini();
  return ret;
}
