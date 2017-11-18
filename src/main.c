#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "argparser.h"
#include "segmenter.h"
#include "segmentation.h"


int
main(int argc, char **argv)
{

  worm_data_t worm_data = { 0 };
  prog_args_t prog_args;

  if (parse_arguments(argc, argv, &prog_args) < 0) {
    fprintf(stderr, "Failed to parse arguments\n");
    exit(1);
  }

  if (log_init(prog_args.verbosity, 0, prog_args.logfile) < 0) {
    exit (1);
  }

  LOG_INFO("Starting Segmenter");
  find_centroid("data/0000000.jpg", &worm_data);
  //find_centroid("kathu.jpg", &worm_data);

  log_fini();

}

