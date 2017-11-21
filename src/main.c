#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "log.h"
#include "argparser.h"
#include "segmenter.h"

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

  if (log_init(task.verbosity, 0, task.logfile) < 0) {
    exit (1);
  }

  LOG_INFO("Starting Segmenter");
  if (dispatch_segmenter_tasks(&task) == 0 &&
      task.reports != NULL) {

    /*
     * Print the results returned by all workers.
     */
    sprintf(file_path, "%s/%s", task.output_dir, task.outfile);
    write_output(file_path, task.reports, task.count);
  } else {
    ret = -1;
  }

  log_fini();

  return ret;
}
