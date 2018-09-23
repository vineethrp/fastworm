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

int MPI_Report_Struct_Type(MPI_Datatype *output_type);
int mpi_master(segment_task_t *task, int nr_tasks, MPI_Datatype *report_type);
int mpi_slave(int taskid, segment_task_t *task, MPI_Datatype *report_type);

static int input[2]; // { starting_frame, nr_frames }
static int output[3]; // { status, starting_frame, nr_frames }

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

  MPI_Report_Struct_Type(&report_type);

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
MPI_Report_Struct_Type(MPI_Datatype *report_type)
{
  MPI_Datatype report_struct;
  MPI_Datatype type[4] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT };
  int blocklen[4] = {1, 1, 1, 1}; 
  MPI_Aint disp[4];

  disp[0] = offsetof(report_t, frame_id);
  disp[1] = offsetof(report_t, centroid_x);
  disp[2] = offsetof(report_t, centroid_y);
  disp[3] = offsetof(report_t, area);

  MPI_Type_create_struct(4, blocklen, disp, type, &report_struct);
  MPI_Type_create_resized(report_struct, 0, sizeof(report_t), report_type);
  MPI_Type_commit(report_type);

  return 0;
}

int
mpi_master(segment_task_t *task, int nr_tasks, MPI_Datatype *report_type)
{
  MPI_Status status;
  char file_path[PATH_MAX];
  int next_frame = 0;
  int frames_per_task;
  int spilled_frames;
  int actual_nr_frames;

  if (task == NULL) {
    LOG_ERR("Invalid task structure");
    return -1;
  }

  LOG_INFO("MPI master task starting");
  actual_nr_frames = task->nr_frames;
  task->reports = (report_t *) calloc(actual_nr_frames, sizeof(report_t));
  if (task->reports == NULL) {
    LOG_ERR("Failed to allocate reports!");
    return -1;
  }

  if (task->nr_frames < nr_tasks)
    nr_tasks = task->nr_frames;

  frames_per_task = task->nr_frames / nr_tasks;
  spilled_frames = task->nr_frames % nr_tasks;
  LOG_XX_DEBUG("nr_tasks=%d, frames_per_task=%d, spilled_frames=%d, nr_frames=%d",
      nr_tasks, frames_per_task, spilled_frames, task->nr_frames);

  /*
   * Master task takes first frames_per_task frames to process
   * So first slave's starting frame_is is frames_per_task.
   */
  next_frame = frames_per_task;

  /*
   * If there are are more frames after dividing equally, we share
   * the load by one more each to the slave tasks.
   */
  if (spilled_frames)
    frames_per_task++;

  // Send Job details to slaves.
  for (int i = 1; i < nr_tasks; i++) {
    input[0] = next_frame;
    input[1] = frames_per_task;

    LOG_XX_DEBUG("MPI Sending task to slave %d: start_frame=%d, nr_tasks=%d",
        i, next_frame, frames_per_task);

    next_frame += frames_per_task;
    spilled_frames--;
    if (spilled_frames == 0)
      frames_per_task--;
    MPI_Send(input, 2, MPI_INT, i, TAG_INPUT, MPI_COMM_WORLD);
  }

  // Master's piece of the task.
  task->base = task->start = 0;
  task->nr_frames = frames_per_task;
  LOG_XX_DEBUG("MPI master task details: start_frame=%d, nr_frames=%di, nr_tasks=%d",
      task->start, task->nr_frames, task->nr_tasks);
  dispatch_segmenter_tasks_static(task);

  // Receive the job output.
  for (int i = 1; i < nr_tasks; i++) {
    MPI_Recv(output, 3, MPI_INT, i, TAG_OUTPUT, MPI_COMM_WORLD, &status);
    LOG_XX_DEBUG("MPI Receiving result from slave %d: status=%d, start_frame=%d, nr_frames=%d",
        i, output[0], output[1], output[2]);

    MPI_Recv(task->reports + output[1], output[2], *report_type, i, TAG_REPORT, MPI_COMM_WORLD, &status);
  }

  LOG_XX_DEBUG("MPI Master writing output file!");
  sprintf(file_path, "%s/%s", task->output_dir, task->outfile);
  write_output(file_path, task->reports, actual_nr_frames);
  return 0;
}

int
mpi_slave(int taskid, segment_task_t *task, MPI_Datatype *report_type)
{
  MPI_Status status;

  LOG_INFO("MPI slave %d starting..", taskid);
  MPI_Recv(input, 2, MPI_INT, TASK_MASTER, TAG_INPUT, MPI_COMM_WORLD, &status);

  task->base = task->start = input[0];
  task->nr_frames = input[1];
  LOG_XX_DEBUG("MPI slave %d: start_frame=%d, nr_frames=%d, nr_tasks=%d",
      taskid, input[0], input[1], task->nr_tasks);

  task->reports = (report_t *) calloc(task->nr_frames, sizeof(report_t));
  if (task->reports == NULL) {
    LOG_ERR("Failed to allocate reports!");
    return -1;
  }
  dispatch_segmenter_tasks_static(task);

  output[0] = 0;
  output[1] = task->base;
  output[2] = task->nr_frames;
  MPI_Send(output, 3, MPI_INT, TASK_MASTER, TAG_OUTPUT, MPI_COMM_WORLD);
  MPI_Send(task->reports, task->nr_frames, *report_type, TASK_MASTER, TAG_REPORT, MPI_COMM_WORLD);

  return 0;
}
