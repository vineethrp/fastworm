#pragma once

#include <stdbool.h>
#include <limits.h>

#define MAX_PIXEL_VALUE 255

#define MAX_IMG_WIDTH 3840
#define MAX_IMG_HEIGHT  2160
#define MAX_IMGBUF_SZ  (MAX_IMG_WIDTH * MAX_IMG_HEIGHT)

typedef struct segdata_s {
  char filename[PATH_MAX];
  char greyscale_filename[PATH_MAX];
  char blur_filename[PATH_MAX];
  char threshold_filename[PATH_MAX];
  unsigned char *blur_data;
  bool *threshold_data;
  unsigned char *tmp_data;
  int *integral_data;
  unsigned char *img_data;
  bool debug_imgs;
  int width;
  int height;
  int channels;
  int centroid_x;
  int centroid_y;
  int area;
} segdata_t;

int segdata_init(segment_task_t *args, char *filename, segdata_t *segdata);
int segdata_reset(segdata_t *segdata, char *filename);
void segdata_fini(segdata_t *segdata);

int segdata_process(segdata_t *segdata);
