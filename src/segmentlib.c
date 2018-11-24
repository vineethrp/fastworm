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
#include "segmentlib.h"

#define MAX_IMG_WIDTH 3840
#define MAX_IMG_HEIGHT 2160
#define MAX_IMGBUF_SZ (MAX_IMG_WIDTH * MAX_IMG_HEIGHT)
static unsigned char blur_data[MAX_IMGBUF_SZ];
static unsigned char tmp_data[MAX_IMGBUF_SZ];
static bool threshold_data[MAX_IMGBUF_SZ];
static int integral_data[MAX_IMGBUF_SZ];

/*
 * Wrapper for a single frame segmentation.
 */

report_t
segment_frame(int x, int y, int frame, int padding,
    int minarea, int maxarea, int srch_winsz, const char *input_dir)
{
  segdata_t segdata = { 0 };
  report_t report;
  segment_task_t task = { 0 };
  work_t w = { 0 };

  // Failure case
  report.area = -1;

  task.minarea = minarea;
  task.maxarea = maxarea;
  task.srch_winsz =  srch_winsz;
  if (validate_options(&task) < 0) {
    fprintf(stderr, "Failed to populate task structure!\n");
    return report;
  }

  strcpy(task.input_dir, input_dir);
  w.centroid_x = x;
  w.centroid_y = y;
  w.frame = task.frame = frame;
  w.padding = task.padding = padding;
  if (do_segment_frame(&task, w, &segdata, true, &report,
                    blur_data, tmp_data, threshold_data, integral_data) < 0) {
    fprintf(stderr, "Failed to segment the frame");
  }

  return report;
}
