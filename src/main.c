#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "log.h"
#include "argparser.h"
#include "segmenter.h"

/*
 * Entry point for segmenter.
 */

int
main(int argc, char **argv)
{
  int ret = 0;
  char file_path[PATH_MAX];
  segment_task_t task = { 0 };

  if (parse_arguments(argc, argv, &task) < 0) {
    fprintf(stderr, "Failed to parse arguments\n");
    exit(1);
  }

  // create output directory
  errno = 0;
  if (mkdir(task.output_dir, 0750) < 0 && errno != EEXIST) {
    fprintf(stderr, "Failed to create the output directory!\n");
    exit(1);
  }

  sprintf(file_path, "%s/%s", task.output_dir, task.logfile);
  if (log_init(task.verbosity, 0, file_path) < 0) {
    exit (1);
  }

  LOG_INFO("Starting Segmenter");
  if (dispatch_segmenter_tasks(&task) == 0 &&
      task.reports != NULL) {

    /*
     * Print the results returned by all workers.
     */
    sprintf(file_path, "%s/%s", task.output_dir, task.outfile);
    write_output(file_path, task.reports, task.nr_frames);
  } else {
    ret = -1;
  }

  log_fini();

  return ret;
}
