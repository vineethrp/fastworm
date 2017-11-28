#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <argp.h>

#include "segmenter.h"
#include "log.h"
#include "argparser.h"

const char *argp_program_version = SEGMENTER_VERSION_STR;
const char *argp_program_bug_address = "vpillai3@hawk.iit.edu";

static char doc[] = "segmenter -- Application to extract worm centroid from image";

void init_options(prog_args_t *prog_args);
int validate_options(prog_args_t *prog_args);
int parse_options(int key, char *arg, struct argp_state *state);

struct argp_option options[] = {
  {"project",           'P', "NAME",      0, "The name of the project to process."},
  {"input-dir",         'i', "PATH",      0, "Path to input images."},
  {"output-dir",        'o', "PATH",      0, "Output file path."},
  {"output-file",       'O', "FILE NAME", 0, "Output file path."},
  {"padding",           'p', "NUMBER",    0, "Number of digits in the file name."},
  {"start-frame",       's', "NUMBER",    0, "Start number of the first frame."},
  {"frames",            'f', "FRAMES",    0, "Number of frames to be processed."},
  {"extension",         'e', "STRING",    0, "Extension of the image files."},
  {"minarea",           'a', "NUMBER",    0, "The lower bound for a candidate worm component."},
  {"maxarea",           'A', "NUMBER",    0, "The upper bound for a candidate worm component."},
  {"search_winsz",      'S', "NUMBER",    0, "Width and height of crop area."},
  {"blur_winsz",        'b', "NUMBER",    0, "Width and height of the sliding window in the box blur."},
  {"thresh_winsz",      't', "NUMBER",    0, "Width and height of the sliding window in the dynamic threshold."},
  {"thresh_ratio",      'T', "FLOAT",     0, "Pixel intensity."},
  {"jobs",              'j', "NUMBER",    0, "Number of concurrent jobs(threads) to run locally"},
  {"logfile",           'l', "FILE NAME", 0, "Path to log file."},
  {"verbose",           'v', 0,           0, "Produce verbose output."},
  {"debug",             'd', 0,           0, "Enable creation of debug images"},
  {"dynamic_threshold", 'D', 0,           0, "Use dynamic thresholding algorithm"},
  { 0 }
};

struct argp argp = {options, parse_options, 0, doc};

int
parse_arguments(int argc, char *argv[], prog_args_t *prog_args)
{
  init_options(prog_args);

  argp_parse (&argp, argc, argv, 0, 0, prog_args);
  if (validate_options(prog_args) < 0) {
    return -1;
  }

  return 0;
}

int
parse_options(int key, char *arg, struct argp_state *state)
{
  prog_args_t *prog_args = (prog_args_t *)state->input;

  switch (key) {
    case 'P':
      strcpy(prog_args->project, arg);
      break;
    case 'i':
      strcpy(prog_args->input_dir, arg);
      break;
    case 'o':
      strcpy(prog_args->output_dir, arg);
      break;
    case 'O':
      strcpy(prog_args->outfile, arg);
      break;
    case 'p':
      prog_args->padding = atoi(arg);
      break;
    case 'f':
      prog_args->nr_frames = atoi(arg);
      break;
    case 's':
      prog_args->base = atoi(arg);
      break;
    case 'e':
      strcpy(prog_args->ext, arg);
      break;
    case 'a':
      prog_args->minarea = atoi(arg);
      break;
    case 'A':
      prog_args->maxarea = atoi(arg);
      break;
    case 'S':
      prog_args->srch_winsz = atoi(arg);
      break;
    case 'b':
      prog_args->blur_winsz = atoi(arg);
      break;
    case 't':
      prog_args->thresh_winsz = atoi(arg);
      break;
    case 'T':
      prog_args->thresh_ratio = atof(arg);
      break;
    case 'j':
      prog_args->nr_tasks = atoi(arg);
      break;
    case 'l':
      strcpy(prog_args->logfile, arg);
      break;
    case 'D':
      prog_args->dynamic_threshold = true;
    case 'v':
      prog_args->verbosity++;
      break;
    case 'd':
      prog_args->debug_imgs = true;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

void
init_options(prog_args_t *prog_args)
{
  bzero(prog_args, sizeof(prog_args_t));
}

int
validate_options(prog_args_t *prog_args)
{
  if (prog_args->project[0] == 0) {
    strcpy(prog_args->project, PROGS_ARGS_DEFAULT_PROJECT);
  }

  if (prog_args->input_dir[0] == 0) {
    strcpy(prog_args->input_dir, PROG_ARGS_DEFAULT_INPUT_DIR);
  }

  if (prog_args->output_dir[0] == 0) {
    strcpy(prog_args->output_dir, PROG_ARGS_DEFAULT_OUTPUT_DIR);
  }

  if (prog_args->outfile[0] == 0) {
    strcpy(prog_args->outfile, PROG_ARGS_DEFAULT_OUTPUT_FILE);
  }

  if (prog_args->ext[0] == 0) {
    strcpy(prog_args->ext, PROG_ARGS_DEFAULT_EXT);
  }

  if (prog_args->logfile[0] == 0) {
    strcpy(prog_args->logfile, PROG_ARGS_DEFAULT_LOGFILE);
  }

  if (prog_args->padding == 0) {
    prog_args->padding = PROG_ARGS_DEFAULT_PADDING;
  }

  if (prog_args->nr_frames == 0) {
    prog_args->nr_frames = PROG_ARGS_DEFAULT_FRAMES;
  }

  if (prog_args->minarea == 0) {
    prog_args->minarea = PROG_ARGS_DEFAULT_MINAREA;
  }

  if (prog_args->maxarea == 0) {
    prog_args->maxarea = PROG_ARGS_DEFAULT_MAXAREA;
  }

  if (prog_args->blur_winsz == 0) {
    prog_args->blur_winsz = PROG_ARGS_DEFAULT_BLUR_WINSZ;
  }

  if (prog_args->srch_winsz == 0) {
    prog_args->srch_winsz = PROG_ARGS_DEFAULT_SRCH_WINSZ;
  }

  if (prog_args->thresh_winsz == 0) {
    prog_args->thresh_winsz = PROG_ARGS_DEFAULT_THRESH_WINSZ;
  }

  if (prog_args->thresh_ratio == 0) {
    prog_args->thresh_ratio = PROG_ARGS_DEFAULT_THRESH_RATIO;
  }

  if (prog_args->nr_tasks == 0)
    prog_args->nr_tasks = sysconf(_SC_NPROCESSORS_ONLN);
  if (prog_args->nr_tasks > TASKS_MAX)
    prog_args->nr_tasks = TASKS_MAX;

  if (prog_args < LOG_ERR) {
    prog_args->verbosity = LOG_INFO;
  }

  if (prog_args->verbosity > LOG_INFO) {
    fprintf(stdout, "Program options:\n");
    fprintf(stdout, "Project: %s, Input dir: %s, Output dir: %s\n",
        prog_args->project, prog_args->input_dir, prog_args->output_dir);
    fprintf(stdout, "Output file: %s, log file: %s\n",
        prog_args->outfile, prog_args->logfile);
    fprintf(stdout, "padding: %d, start_frame: %d, frames: %d\n",
        prog_args->padding, prog_args->base, prog_args->nr_frames);
    fprintf(stdout, "minarea: %d, maxarea: %d, blur_winsz: %d\n",
        prog_args->minarea, prog_args->maxarea, prog_args->blur_winsz);
    fprintf(stdout, "srch_winsz: %d, thresh_winsz: %d, thresh_ratio: %f\n",
        prog_args->srch_winsz, prog_args->thresh_winsz, prog_args->thresh_ratio);
    fprintf(stdout, "nr_tasks: %d\n", prog_args->nr_tasks);
    fprintf(stdout, "debug_imgs: %d, verbosity: %d\n",
        prog_args->debug_imgs, prog_args->verbosity);
  }

  return 0;
}

