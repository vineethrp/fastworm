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

int
segment_frame(int x, int y, int frame, int padding, const char *input_dir)
{
  int ret = -1;
  segdata_t segdata = { 0 };
  report_t report;
  segment_task_t task = { 0 };
  work_t w = { 0 };

  if (validate_options(&task) < 0) {
    fprintf(stderr, "Failed to populate task structure!\n");
    return -1;
  }

  strcpy(task.input_dir, input_dir);
  w.centroid_x = x;
  w.centroid_y = y;
  w.frame = task.frame = frame;
  w.padding = task.padding = padding;
  if (do_segment_frame(&task, w, &segdata, true, &report,
                    blur_data, tmp_data, threshold_data, integral_data) == 0) {
    printf("%d %d %d %d\n",
        report.frame_id, report.centroid_x, report.centroid_y, report.area);
    ret = 0;
  } else {
    fprintf(stderr, "Failed to segment the frame");
  }

  return ret == 0 ? report.area : ret;
}
