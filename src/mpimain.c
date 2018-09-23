#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "work_queue.h"
#include "segmenter.h"
#include "argparser.h"
#include "log.h"

#define TASK_MASTER 0
#define TAG_INPUT 1
#define TAG_OUTPUT 1
#define TAG_REPORT 1

int mpi_master(segment_task_t *task, int nr_tasks, MPI_Datatype *report_type);
int mpi_slave(int taskid, segment_task_t *task, MPI_Datatype *report_type);

typedef struct output_s {
  int status;
  report_t report;
} output_t;

static work_t input; // { frame_id, padding, x, y }
static output_t output; // { status, frame_id, x, y, area }

/*
 * Entry point for msegmenter.
 */

int
main(int argc, char *argv[])
{
  MPI_Datatype report_type;
  int  nr_tasks, taskid, len;
  int  ret = -1;
  char hostname[MPI_MAX_PROCESSOR_NAME];
  char logfile[PATH_MAX];
  segment_task_t task = { 0 };

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nr_tasks);
  MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
  MPI_Get_processor_name(hostname, &len);

  if (segment_task_init(argc, argv, &task, taskid == TASK_MASTER) < 0) {
    fprintf(stderr, "Failed to parse arguments\n");
    return -1;
  }

  // create output directory
  errno = 0;
  if (mkdir(task.output_dir, 0750) < 0 && errno != EEXIST) {
    fprintf(stderr, "Failed to create the output directory!\n");
    goto out;
  }

  sprintf(logfile, "%s/task-%d_%s", task.output_dir, taskid, task.logfile);
  if (log_init(task.verbosity, 0, logfile) < 0) {
    goto out;
  }

  // Override nr_tasks from MPI
  task.nr_tasks = nr_tasks;

  LOG_INFO("Starting Segmenter");
  if (taskid == TASK_MASTER) {
    ret = mpi_master(&task, nr_tasks, &report_type);
  } else {
    ret = mpi_slave(taskid, &task, &report_type);
  }

  ret = 0;
out:
  log_fini();
  segment_task_fini(&task);
  MPI_Finalize();
  return ret;
}

int
mpi_master_distribute(void *data, work_t w, bool last)
{
  MPI_Status status;
  segment_task_t *task = (segment_task_t *)data;

  if (task == NULL) {
    return -1;
  }

  if (last) {
    input.frame = -1;
    for (int i = 0; i < task->nr_tasks - 1; i++) {
      MPI_Recv(&output, 5, MPI_INT, MPI_ANY_SOURCE, TAG_OUTPUT, MPI_COMM_WORLD, &status);
      if (output.status <= 0) {
        task->reports[output.report.frame_id] = output.report;
      }
      MPI_Send(&input, 4, MPI_INT, status.MPI_SOURCE, TAG_INPUT, MPI_COMM_WORLD);
    }
    return 0;
  }

  // Receive request/result and send new job
  MPI_Recv(&output, 5, MPI_INT, MPI_ANY_SOURCE, TAG_OUTPUT, MPI_COMM_WORLD, &status);
  LOG_XX_DEBUG("MPI Receiving from slave %d: status=%d, frame_id=%d, x=%d, y=%d, area=%d",
        status.MPI_SOURCE, output.status, output.report.frame_id,
        output.report.centroid_x, output.report.centroid_y, output.report.area);

  // Status = 0 -> Segmentation Success
  // Status < 0 -> Segmentation Failure
  // Status = 1 -> No result, just asking for new job.
  if (output.status <= 0) {
        task->reports[output.report.frame_id] = output.report;
  }

  input = w;

  MPI_Send(&input, 4, MPI_INT, status.MPI_SOURCE, TAG_INPUT, MPI_COMM_WORLD);

  return 0;
}

int
mpi_master(segment_task_t *task, int nr_tasks, MPI_Datatype *report_type)
{
  char file_path[PATH_MAX];

  if (task == NULL) {
    LOG_ERR("Invalid task structure");
    return -1;
  }

  LOG_INFO("MPI master task starting");

  if (task->nr_frames < nr_tasks)
    nr_tasks = task->nr_frames;

  process_infile(task->input_file, mpi_master_distribute,
                  (void *)task, task->nr_frames);

  LOG_XX_DEBUG("MPI Master writing output file!");
  sprintf(file_path, "%s/%s", task->output_dir, task->outfile);
  write_output(file_path, task->reports, task->nr_frames);
  return 0;
}

int
mpi_slave(int taskid, segment_task_t *task, MPI_Datatype *report_type)
{
  MPI_Status status;
  char file_path[PATH_MAX];
  bool need_segdata_init = true;
  segdata_t segdata = { 0 };

  LOG_INFO("MPI slave %d starting..", taskid);
  // Set status to 1 fro first job.
  output.status = 1;
  while (1) {
    int ret = 0;
    // Send request to master
    MPI_Send(&output, 5, MPI_INT, TASK_MASTER, TAG_OUTPUT, MPI_COMM_WORLD);

    // Recv job
    MPI_Recv(&input, 4, MPI_INT, TASK_MASTER, TAG_INPUT, MPI_COMM_WORLD, &status);

    if (input.frame < 0) {
      // Master signalled end.
      LOG_INFO("(SLave %d): Master send FINI", taskid);
      break;
    }

    if (filepath(input.padding, input.frame, task->input_dir, task->ext, file_path) < 0) {
      LOG_ERR("Failed to create filepath!");
      break;
    }
    if (need_segdata_init) {
      if (segdata_init(task, file_path, &segdata, input.centroid_x, input.centroid_y) < 0) {
        LOG_ERR("Failed to initialize segdata");
        ret = -1;
      }
      need_segdata_init = false;
    } else {
      if (segdata_reset(&segdata, file_path, input.centroid_x, input.centroid_y) < 0) {
        LOG_ERR("Failed to reset segdata");
        ret = -1;
      }
    }

    if (ret == 0) {
      segdata_process(&segdata);
    }
    output.status = ret;
    output.report.frame_id = input.frame;
    output.report.centroid_x =  segdata.centroid_x;
    output.report.centroid_y =  segdata.centroid_y;
    output.report.area =  segdata.area;
  }

  return 0;
}
