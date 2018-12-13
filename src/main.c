#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <pthread.h>

#include "log.h"
#include "argparser.h"
#include "work_queue.h"
#include "segmenter.h"

/*
 * Entry point for segmenter.
 */

int
main(int argc, char **argv)
{
  int ret = -1;
  char file_path[PATH_MAX + NAME_MAX];
  segment_task_t task = { 0 };

  if (segment_task_init(argc, argv, &task, true) < 0) {
    fprintf(stderr, "Failed to initialize\n");
    return ret;
  }

  // create output directory
  errno = 0;
  if (mkdir(task.output_dir, 0750) < 0 && errno != EEXIST) {
    fprintf(stderr, "Failed to create the output directory!: %s\n", strerror(errno));
    goto out;
  }

  snprintf(file_path, PATH_MAX + NAME_MAX, "%s/%s", task.output_dir, task.logfile);
  if (log_init(task.verbosity, 0, file_path) < 0) {
    goto out;
  }

  LOG_INFO("Starting Segmenter");
  if (dispatch_segmenter_tasks(&task) == 0 &&
      task.reports != NULL) {

    /*
     * Print the results returned by all workers.
     */
    snprintf(file_path, PATH_MAX + NAME_MAX, "%s/%s", task.output_dir, task.outfile);
    write_output(file_path, task.reports, task.nr_frames);
  }

  ret = 0;

out:
  segment_task_fini(&task);

  log_fini();

  return ret;
}
