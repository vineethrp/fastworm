#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>


#include "segmenter.h"

const char *argp_program_version = SEGMENTER_VERSION_STR;
const char *argp_program_bug_address = "vpillai3@hawk.iit.edu";

static char doc[] = "segmenter -- A program to extract worm centroid from image";

int validate_options(prog_args_t *);
int parse_options(int, char *, struct argp_state *);

int
main(int argc, char **argv)
{

  prog_args_t prog_args;
  bzero(&prog_args, sizeof(prog_args));
  struct argp_option options[] = {
    {"project",         'P', "NAME",       0,                    "The name of the project to process."},
    {"input",           'i', "PATH",       0,  "Path to input images."},
    {"output",          'o', "FILE PATH",  0,  "Output file path."},
    {"padding",         'p', "NUMBER",     0,  "Number of digits in the file name."},
    {"frames",          'f', "FRAMES",     0,  "Number of frames to be processed."},
    {"extension",       'e', "STRING",     0,  "Extension of the image files."},
    {"width",           'w', "WIDTH",      0,  "The horizontal resolution of the image."},
    {"height",          'h', "HEIGHT",     0,  "The vertical resolution of the image."},
    {"minarea",         'a', "NUMBER",     0,  "The lower bound for a candidate worm component."},
    {"maxarea",        'A', "NUMBER",     0,  "The upper bound for a candidate worm component."},
    {"search_winsz", 's', "NUMBER",  0,  "Width and height of crop area."},
    {"blur_radius",     'b', "NUMBER",  0,  "Width and height of the sliding window in the box blur."},
    {"thresh_winsz", 't', "NUMBER",  0,  "Width and height of the sliding window in the dynamic threshold."},
    {"thresh_ratio",    'T', "FLOAT",      0,  "Pixel intensity."},
    {"logfile",         'l', "FILE PATH",  0,  "Path to log file."},
    {"verbose",         'v', 0, 0,  "Produce verbose output."},
    { 0 }
  };

  struct argp argp = {options, parse_options, 0, doc};

  argp_parse (&argp, argc, argv, 0, 0, &prog_args);
  if (validate_options(&prog_args) < 0) {
    exit(1);
  }

}

int
parse_options(int key, char *arg, struct argp_state *state)
{
  prog_args_t *prog_args = (prog_args_t *)state->input;

  switch (key) {
    /*
    case ARGP_KEY_END:
      if (state->arg_num < 1) {
        fprintf(stderr, "Insufficient number of arguments!\n");
        argp_usage(state);
      }
      break;
      */
    case 'P':
      strcpy(prog_args->project, arg);
      break;
    case 'i':
      strcpy(prog_args->input, arg);
      break;
    case 'o':
      strcpy(prog_args->output, arg);
      break;
    case 'p':
      prog_args->padding = atoi(arg);
      break;
    case 'f':
      prog_args->frames = atoi(arg);
      break;
    case 'e':
      strcpy(prog_args->ext, arg);
      break;
    case 'w':
      prog_args->width = atoi(arg);
      break;
    case 'h':
      prog_args->height = atoi(arg);
      break;
    case 'a':
      prog_args->minarea = atoi(arg);
      break;
    case 'A':
      prog_args->maxarea = atoi(arg);
      break;
    case 's':
      prog_args->srch_winsz = atoi(arg);
      break;
    case 'b':
      prog_args->blur_radius = atoi(arg);
      break;
    case 't':
      prog_args->thresh_winsz = atoi(arg);
      break;
    case 'T':
      prog_args->thresh_ratio = atof(arg);
      break;
    case 'l':
      strcpy(prog_args->logfile, arg);
      break;
    case 'v':
      prog_args->verbosity++;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

int
validate_options(prog_args_t *prog_args)
{
  if (prog_args->project[0] == 0) {
    fprintf(stderr, "Project not specified!\n");
    return -1;
  }

  if (prog_args->input[0] == 0) {
    strcpy(prog_args->input, PROG_ARGS_DEFAULT_INPUT);
  }

  if (prog_args->output[0] == 0) {
    strcpy(prog_args->output, PROG_ARGS_DEFAULT_OUTPUT);
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

  if (prog_args->frames == 0) {
    prog_args->frames = PROG_ARGS_DEFAULT_FRAMES;
  }

  if (prog_args->width == 0) {
    prog_args->width = PROG_ARGS_DEFAULT_WIDTH;
  }

  if (prog_args->height == 0) {
    prog_args->height = PROG_ARGS_DEFAULT_HEIGHT;
  }

  if (prog_args->minarea == 0) {
    prog_args->minarea = PROG_ARGS_DEFAULT_MINAREA;
  }

  if (prog_args->maxarea == 0) {
    prog_args->maxarea = PROG_ARGS_DEFAULT_MAXAREA;
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

  if (prog_args->verbosity) {
    fprintf(stdout, "Program options:\n");
    fprintf(stdout, "Project: %s, Input: %s, Output: %s\n",
        prog_args->project, prog_args->input, prog_args->output);
    fprintf(stdout, "padding: %d, frames: %d, width: %d, height: %d\n",
        prog_args->padding, prog_args->frames, prog_args->width, prog_args->height);
    fprintf(stdout, "minarea: %d, maxarea: %d\n",
        prog_args->minarea, prog_args->maxarea);
    fprintf(stdout, "srch_winsz: %d, thresh_winsz: %d, thresh_ratio: %f\n",
        prog_args->srch_winsz, prog_args->thresh_winsz, prog_args->thresh_ratio);
    fprintf(stdout, "verbosity: %d\n", prog_args->verbosity);
  }

  return 0;
}
