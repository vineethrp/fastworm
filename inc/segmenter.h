#pragma once

#include "segmentation.h"

#define SEGMENTER_VERSION_STR "segmenter 0.1.0"

#define EXT_MAX 8

#define DEBUG_DIR "debug"

#define TASKS_MAX 128

typedef struct report_s {
  int frame_id;
  int centroid_y;
  int centroid_x;
  int area;
} report_t;

typedef struct segment_task_s {
  /*
   * Task global data
   */
  char project[NAME_MAX];
  char input_dir[PATH_MAX];
  char output_dir[PATH_MAX];
  char outfile[NAME_MAX];
  char logfile[NAME_MAX];
  char ext[EXT_MAX];
  int padding;
  int minarea;
  int maxarea;
  int srch_winsz;
  int blur_winsz;
  int thresh_winsz;
  float thresh_ratio;
  bool debug_imgs;
  bool dynamic_threshold;
  int nr_tasks;
  int verbosity;

  /*
   * per task data
   */
  int base;       // Starting frame for this process.
  int start;      // Starting frame for this thread
  int nr_frames;  // Number of frames to process for this thread
  report_t *reports;
} segment_task_t;

typedef struct segdata_s {
  char filename[PATH_MAX];
  char greyscale_filename[PATH_MAX];
  char blur_filename[PATH_MAX];
  char threshold_filename[PATH_MAX];
  unsigned char *blur_data;
  bool *threshold_data;
  unsigned char *tmp_data;
  int *integral_data;
  unsigned char *img_data;
  bool debug_imgs;
  threshold_fn_t thresh_fn;

  /*
   * Segmentation tunables
   */
  int minarea;
  int maxarea;
  int srch_winsz;
  int blur_winsz;
  int thresh_winsz;
  float thresh_ratio;

  /*
   * Image details
   */
  int width;
  int height;
  int channels;

  /*
   * Segmentation result
   */
  int centroid_x;
  int centroid_y;
  int area;
} segdata_t;

int segdata_init(segment_task_t *args, char *filename, segdata_t *segdata);
int segdata_reset(segdata_t *segdata, char *filename);
void segdata_fini(segdata_t *segdata);

int segdata_process(segdata_t *segdata);

int write_output(char *filepath, report_t *reports, int count);
void *task_segmenter(void *data);
int dispatch_segmenter_tasks(segment_task_t *task);
