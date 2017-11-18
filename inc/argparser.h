#pragma once

#define NAME_MAX_LEN  128

typedef struct prog_args_s {
  char project[NAME_MAX_LEN];
  char input[NAME_MAX_LEN];
  char ext[8];
  char output[NAME_MAX_LEN];
  char logfile[NAME_MAX_LEN];
  int padding;
  int frames;
  int width;
  int height;
  int blur_radius;
  int minarea;
  int maxarea;
  int srch_winsz;
  int thresh_winsz;
  float thresh_ratio;
  loglevel_t verbosity;
} prog_args_t;

#define PROG_ARGS_DEFAULT_INPUT "data"
#define PROG_ARGS_DEFAULT_OUTPUT "output.txt"
#define PROG_ARGS_DEFAULT_EXT "jpg"
#define PROG_ARGS_DEFAULT_LOGFILE "segmenter.log"
#define PROG_ARGS_DEFAULT_PADDING 7
#define PROG_ARGS_DEFAULT_FRAMES 1000
#define PROG_ARGS_DEFAULT_WIDTH 640
#define PROG_ARGS_DEFAULT_HEIGHT 480
#define PROG_ARGS_DEFAULT_MINAREA 200
#define PROG_ARGS_DEFAULT_MAXAREA 400
#define PROG_ARGS_DEFAULT_SRCH_WINSZ 100
#define PROG_ARGS_DEFAULT_THRESH_WINSZ 25
#define PROG_ARGS_DEFAULT_THRESH_RATIO 0.9
#define PROG_ARGS_DEFAULT_BLUR_RADIUS 3

int parse_arguments(int argc, char *argv[], prog_args_t *prog_args);

