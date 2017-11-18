#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "log.h"
#include "argparser.h"
#include "segmenter.h"
#include "segmentation.h"

bool debug_run = false;

int
main(int argc, char **argv)
{
  segdata_t segdata = { 0 };;

  prog_args_t prog_args;

  if (parse_arguments(argc, argv, &prog_args) < 0) {
    fprintf(stderr, "Failed to parse arguments\n");
    exit(1);
  }

  if (log_init(prog_args.verbosity, 0, prog_args.logfile) < 0) {
    exit (1);
  }

  LOG_INFO("Starting Segmenter");
  if (segdata_init(&prog_args, "data/0000000.jpg", &segdata) < 0) {
    LOG_ERR("Failed to initialize segdata");
    goto out;
  }
  segdata_process(&segdata);
  segdata_fini(&segdata);
out:

  log_fini();
}

