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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#include "segmenter.h"
#include "log.h"
#include "argparser.h"
#include "segmentation.h"
#include "largestcomponent.h"
#include "profile.h"

static int write_threshold(char *filename,
    bool *threshold_data, unsigned char *data, int w, int h);
static int filepath(int padding, int num, char *prefix, char *ext, char *path);

int
dispatch_segmenter_tasks(segment_task_t *task)
{
  int i = 0;
  pthread_t thrs[TASKS_MAX];
  int frames_per_thread;
  int spilled_tasks, prev_start, prev_nr_frames;
  segment_task_t main_task = { 0 };
  segment_task_t thr_tasks[TASKS_MAX];

  if (task->nr_frames < task->nr_tasks)
    task->nr_tasks = task->nr_frames;

  frames_per_thread = task->nr_frames / task->nr_tasks;

  task->reports = (report_t *) calloc(task->nr_frames, sizeof(report_t));
  if (task->reports == NULL) {
    LOG_ERR("Failed to allocate reports!");
    return -1;
  }

  /*
   * start nr_tasks - 1 threads (this main thread also does job!)
   *
   * Work splitting is like this:
   * frame per thread = total frames / number of threads
   * Main thread takes first frame_per_thread
   * then each thread takes one slice of frame_per_thread.
   * If the number of frames is not equally divisible by number of threads,
   * each thread takes one extra until there are no more spilled over frames.
   * This way, one thread is not overloaded with all the spilled over frames.
   */
  prev_start = task->base;
  prev_nr_frames = frames_per_thread;
  spilled_tasks = task->nr_frames % frames_per_thread;
  for (i = 0; i < task->nr_tasks - 1; i++) {
    thr_tasks[i] = *task;
    thr_tasks[i].start = prev_start + prev_nr_frames;
    thr_tasks[i].nr_frames = frames_per_thread;
    if (spilled_tasks > 0) {
      thr_tasks[i].nr_frames++;
      spilled_tasks--;
    }
    prev_start = thr_tasks[i].start;
    prev_nr_frames = thr_tasks[i].nr_frames;
    LOG_XX_DEBUG("Thread %d: start=%d, nr_frames=%d", i, thr_tasks[i].start, thr_tasks[i].nr_frames);
    if (pthread_create(&thrs[i], NULL, task_segmenter, (void *)&thr_tasks[i]) < 0) {
      LOG_ERR("Failed to create thread %d!", i);
      goto err;
    }
  }

  /*
   * Rest of the work in Main thread!
   */
  main_task = *task;
  main_task.nr_frames = frames_per_thread;
  main_task.start = task->base;
  LOG_XX_DEBUG("Main Thread: start=%d, nr_frames=%d", main_task.start, main_task.nr_frames);
  task_segmenter(&main_task);

  /*
   * Wait for all worker threads!
   */
  for (i = 0; i < task->nr_tasks - 1; i++) {
    int *ret_val;
    pthread_join(thrs[i], (void *)&ret_val);
    if ((long long)ret_val != 0) {
      LOG_ERR("Thread %d failed!", i);
      goto err;
    }
  }
  return 0;

err:
  free(task->reports);
  return -1;
}

void *
task_segmenter(void *data)
{
  int ret = 0;
  int i, last;
  char file_path[PATH_MAX];
  segment_task_t *task = NULL;
  segdata_t segdata = { 0 };

  task = (segment_task_t *)data;
  last = task->start + task->nr_frames;

  for (i = task->start; i < last; i++) {
    int report_index = i - task->base;
    ret = -1;
    if (filepath(task->padding, i, task->input_dir, task->ext, file_path) < 0) {
      LOG_ERR("Failed to create filepath!");
      break;
    }
    if (i == task->start) {
      if (segdata_init(task, file_path, &segdata) < 0) {
        LOG_ERR("Failed to initialize segdata");
        break;
      }
    } else {
      if (segdata_reset(&segdata, file_path) < 0) {
        LOG_ERR("Failed to reset segdata");
        break;
      }
    }
    segdata_process(&segdata);
    task->reports[report_index].frame_id = i;
    task->reports[report_index].centroid_x = segdata.centroid_x;
    task->reports[report_index].centroid_y = segdata.centroid_y;
    task->reports[report_index].area = segdata.area;
    ret = 0;
  }

  segdata_fini(&segdata);
  return (void *)(unsigned long long)ret;
}

int
segdata_init(segment_task_t *args, char *filename, segdata_t *segdata)
{
  size_t imgbuf_sz;
  unsigned char *imgbuf;

  if (segdata == NULL) {
    LOG_ERR("segdata_init: Invalid argument passed!");
    return -1;
  }

  if (segdata->blur_data || segdata->threshold_data ||
      segdata->tmp_data || segdata->integral_data) {
    LOG_ERR("segdata buffers seems to be partially initialized!");
    return -1;
  }
  bzero(segdata, sizeof(segdata_t));
  segdata->debug_imgs = args->debug_imgs;

  if (segdata_reset(segdata, filename) < 0)
    goto err;

  /*
   * Initialize all the buffers.
   */
  imgbuf_sz = segdata->width * segdata->height;
  imgbuf = (unsigned char *) calloc(2 * imgbuf_sz, sizeof(unsigned char));
  if (imgbuf == NULL) {
    LOG_ERR("Failed to allocate Image buffer data");
    goto err;
  }
  segdata->blur_data = imgbuf;
  segdata->tmp_data = imgbuf + imgbuf_sz;

  segdata->threshold_data = (bool *)calloc(imgbuf_sz, sizeof(bool));
  if (segdata->threshold_data == NULL) {
    LOG_ERR("Failed to allocate threshold_data");
    goto err;
  }
  segdata->integral_data = (int *)calloc(imgbuf_sz, sizeof(int));
  if (segdata->integral_data == NULL) {
    LOG_ERR("Failed to allocate threshold_data");
    goto err;
  }

  segdata->minarea = args->minarea;
  segdata->maxarea = args->maxarea;
  segdata->srch_winsz = args->srch_winsz;
  segdata->blur_winsz = args->blur_winsz;
  segdata->thresh_winsz = args->thresh_winsz;
  segdata->thresh_ratio = args->thresh_ratio;
  if (!args->dynamic_threshold)
    segdata->thresh_ratio -= 0.03;

  if (args->dynamic_threshold)
    segdata->thresh_fn = dynamic_threshold;
  else
    segdata->thresh_fn = simple_threshold;

  return 0;

err:
  segdata_fini(segdata);
  return -1;

}

void segdata_fini(segdata_t *segdata)
{
  if (segdata == NULL)
    return;

  if (segdata->img_data)
    free(segdata->img_data);
  if (segdata->blur_data)
    free(segdata->blur_data);
  if (segdata->threshold_data)
    free(segdata->threshold_data);
  if (segdata->integral_data)
    free(segdata->integral_data);
  bzero(segdata, sizeof(segdata_t));
}

int
segdata_reset(segdata_t *segdata, char *filename)
{
  char *fname = NULL;
  DEFINE_PROFILE_VARS

  if (segdata->img_data != NULL) {
    free(segdata->img_data);
    segdata->height = segdata->width = segdata->channels = 0;
  }

  LOG_DEBUG("Loading %s to memory", filename);

  START_PROFILE
  segdata->img_data = stbi_load(filename, &segdata->width,
      &segdata->height, &segdata->channels, 1);
  END_PROFILE("imgload")

  if (segdata->img_data == NULL) {
    LOG_ERR("Failed to load the image to memory!");
    goto err;
  }
  LOG_DEBUG("Image details(%s): w=%d, h=%d, n=%d",
      filename, segdata->width, segdata->height, segdata->channels);

  if (strcpy(segdata->filename, filename) == NULL) {
    LOG_ERR("Failed to copy filename!");
    goto err;
  }


  if (!segdata->debug_imgs)
    return 0;

  /*
   * Reset debug fields.
   */
  // create debug folder
  errno = 0;
  if (mkdir(DEBUG_DIR, 0750) < 0 && errno != EEXIST) {
    LOG_ERR("Failed to create the debug directory!");
    goto err;
  }

  // get the file name from path
  fname = strrchr(filename, '/');
  if (fname != NULL)
    fname++;
  else
    fname = filename;

  if (sprintf(segdata->greyscale_filename, DEBUG_DIR"/gs_%s", fname) < 0) {
    LOG_ERR("Failed to construct greyscale_filename!");
    goto err;
  }
  stbi_write_jpg(segdata->greyscale_filename, segdata->width,
      segdata->height, 1, segdata->img_data, 0);

  if (sprintf(segdata->blur_filename, DEBUG_DIR"/blur_%s", fname) < 0) {
    LOG_ERR("Failed to construct blur_filename!");
    goto err;
  }
  if (sprintf(segdata->threshold_filename, DEBUG_DIR"/thresh_%s", fname) < 0) {
    LOG_ERR("Failed to construct threshold_filename!");
    goto err;
  }

  return 0;
err:
  segdata_fini(segdata);
  return -1;
}

int segdata_process(segdata_t *segdata)
{
  int w, h;
  int x1, x2, y1, y2;
  connected_component_t largest;
  int offset = segdata->srch_winsz / 2;
  DEFINE_PROFILE_VARS

  if (segdata == NULL) {
    LOG_ERR("Inavlid segdata!");
    return -1;
  }
  w = segdata->width;
  h = segdata->height;
  LOG_DEBUG("Processing Image %s", segdata->filename);

  if (segdata->centroid_x != 0 && segdata->centroid_y != 0) {
    x1 = segdata->centroid_x - offset;
    y1 = segdata->centroid_y - offset;
    x2 = segdata->centroid_x + offset;
    y2 = segdata->centroid_y + offset;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > h) x2 = h;
    if (y2 > w) y2 = w;
  } else {
    x1 = y1 = 0;
    x2 = h, y2 = w;
  }

  START_PROFILE
  if (greyscale_blur(segdata->img_data, w, h, x1, y1, x2, y2,
        segdata->blur_winsz, segdata->tmp_data, segdata->blur_data) < 0) {
    LOG_ERR("Failed to blur the image");
    return -1;
  }
  END_PROFILE("blur")
  LOG_DEBUG("greyscal_blur complete");

  if (segdata->debug_imgs) {
    stbi_write_jpg(segdata->blur_filename, segdata->width,
        segdata->height, 1, segdata->blur_data, 0);
  }

  START_PROFILE
  if (segdata->thresh_fn(segdata->blur_data, w, h, x1, y1, x2, y2,
        segdata->thresh_winsz, segdata->thresh_ratio,
        segdata->integral_data, segdata->threshold_data) < 0) {
    LOG_ERR("Thresholding failed!");
    return -1;
  }
  END_PROFILE("threshold")
  LOG_DEBUG("Threshold complete");

  if (segdata->debug_imgs) {
    write_threshold(segdata->threshold_filename,
        segdata->threshold_data, segdata->tmp_data, w, h);
  }

  START_PROFILE
  largest = largest_component(segdata->threshold_data,
      x1, y1, x2, y2, w, h);
  END_PROFILE("largestcomponent")
  LOG_DEBUG("largestcomponent completed");

  if (largest.count <= 0) {
    if (segdata->centroid_x > 0 && segdata->centroid_y > 0) {
      /*
       * cropped image parsing failed.
       * Lets do full image.
       */
      LOG_WARN("Cropped Image threshold failed for %s\n", segdata->filename);
      segdata->centroid_x = 0;
      segdata->centroid_y = 0;
      return segdata_process(segdata);
    }
    LOG_ERR("Thresholding failed for %s", segdata->filename);
    return -1;
  }

  segdata->centroid_x = largest.total_x / largest.count;
  segdata->centroid_y = largest.total_y / largest.count;
  segdata->area = largest.count;
  LOG_INFO("%d %d %d",
      segdata->centroid_x, segdata->centroid_y      ,
      largest.count);

  return 0;
}

static int
write_threshold(char *filename, bool *threshold_data,
    unsigned char *data, int w, int h)
{
  int i, j;

  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      if (*(threshold_data + (i * w) + j) == true) {
        *(data + (i * w) + j) = 255;
      } else {
        *(data + (i * w) + j) = 0;
      }
    }
  }
  stbi_write_jpg(filename, w, h, 1, data, 0);

  return 0;
}

static int
filepath(int padding, int num, char *prefix, char *ext, char *path)
{
  char format[NAME_MAX];
  if (path == NULL)
    return -1;

  if (sprintf(format, "%s/%%0%dd.%s", prefix, padding, ext) <= 0) {
   return -1;
  }

  if (sprintf(path, format, num) <= 0) {
    return -1;
  }

  return 0;
}

int
write_output(char *filepath, report_t *reports, int nr_frames)
{
  int i;
  FILE *f = fopen(filepath, "w");
  if (f == NULL) {
    fprintf(stderr, "Failed to open output file\n");
    return -1;
  }
  for (i = 0; i < nr_frames; i++) {
    fprintf(f, "%d, %d, %d, %d\n",
        reports[i].frame_id, reports[i].centroid_y,
        reports[i].centroid_x, reports[i].area);
  }
  fclose(f);
  return 0;
}
