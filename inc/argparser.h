#pragma once

/*
 * Alias segment_task_s since the contents 
 * are same.
 */
typedef struct segment_task_s prog_args_t;

#define PROG_ARGS_DEFAULT_INPUT_DIR "data"
#define PROG_ARGS_DEFAULT_OUTPUT_DIR "out"
#define PROG_ARGS_DEFAULT_OUTPUT_FILE "output.txt"
#define PROG_ARGS_DEFAULT_EXT "jpg"
#define PROG_ARGS_DEFAULT_LOGFILE "segmenter.log"
#define PROG_ARGS_DEFAULT_PADDING 7
#define PROG_ARGS_DEFAULT_FRAMES 1000
#define PROG_ARGS_DEFAULT_MINAREA 200
#define PROG_ARGS_DEFAULT_MAXAREA 400
#define PROG_ARGS_DEFAULT_SRCH_WINSZ 100
#define PROG_ARGS_DEFAULT_THRESH_WINSZ 25
#define PROG_ARGS_DEFAULT_THRESH_RATIO 0.9
#define PROG_ARGS_DEFAULT_BLUR_RADIUS 3

int parse_arguments(int argc, char *argv[], prog_args_t *prog_args);

