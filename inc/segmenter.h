#pragma once


#define SEGMENTER_VERSION_STR "segmenter 0.1.0"

#define EXT_MAX 8

#define DEBUG_DIR "debug"

#define THREADS_MAX 128

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
  int num_tasks;
  int verbosity;

  /*
   * per task data
   */
  int base;   // Starting frame for this process.
  int start;  // Starting frame for this thread
  int count;  // Number of frames to process for this thread
  report_t *reports;
} segment_task_t;

int write_output(char *filepath, report_t *reports, int count);
void *task_segmenter(void *data);
int dispatch_segmenter_tasks(segment_task_t *task);
