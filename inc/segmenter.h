#pragma once


#define SEGMENTER_VERSION_STR "segmenter 0.1.0"


extern bool debug_run;
#define DEBUG_DIR "debug"
#define EXT_MAX 8

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
  int verbosity;

  /*
   * per task data
   */
  int start;
  int count;
} segment_task_t;

