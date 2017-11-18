#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#include "segmenter.h"
#include "log.h"
#include "argparser.h"
#include "segmentation.h"
#include "largestcomponent.h"

static int threshold_neighbor_sz = 25;
static float threshold_ratio = 0.9;

static int write_threshold(char *filename,
    bool *threshold_data, unsigned char *data, int w, int h);

int
segdata_init(prog_args_t *args, char *filename, segdata_t *segdata)
{

  if (segdata == NULL) {
    LOG_ERR("segdata_init: Invalid argument passed!");
    return -1;
  }

  /*
   * Initialize all the buffers.
   */
  segdata->hblur_data = (unsigned char *)calloc(MAX_IMGBUF_SZ, sizeof(unsigned char));
  if (segdata->hblur_data == NULL) {
    LOG_ERR("Failed to allocate hblur_data");
    goto out;
  }
  segdata->blur_data = (unsigned char *)calloc(MAX_IMGBUF_SZ, sizeof(unsigned char));
  if (segdata->blur_data == NULL) {
    LOG_ERR("Failed to allocate blur_data");
    goto out;
  }
  segdata->tmp_data = (unsigned char *)calloc(MAX_IMGBUF_SZ, sizeof(unsigned char));
  if (segdata->tmp_data == NULL) {
    LOG_ERR("Failed to allocate tmp_data");
    goto out;
  }
  segdata->threshold_data = (bool *)calloc(MAX_IMGBUF_SZ, sizeof(bool));
  if (segdata->threshold_data == NULL) {
    LOG_ERR("Failed to allocate threshold_data");
    goto out;
  }
  segdata->integral_data = (int *)calloc(MAX_IMGBUF_SZ, sizeof(int));
  if (segdata->integral_data == NULL) {
    LOG_ERR("Failed to allocate threshold_data");
    goto out;
  }

  return segdata_reset(segdata, filename);

out:
  segdata_fini(segdata);
  return -1;

}

void segdata_fini(segdata_t *segdata)
{
  if (segdata == NULL)
    return;

  if (segdata->img_data != NULL)
    free(segdata->img_data);
  if (segdata->hblur_data)
    free(segdata->hblur_data);
  if (segdata->blur_data)
    free(segdata->blur_data);
  if (segdata->tmp_data)
    free(segdata->tmp_data);
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
  if (segdata == NULL || segdata->hblur_data == NULL ||
      segdata->blur_data == NULL || segdata->threshold_data == NULL ||
      segdata->tmp_data == NULL || segdata->integral_data == NULL) {
    LOG_ERR("segdata not initialized!");
    goto err;
  }

  if (segdata->img_data != NULL) {
    free(segdata->img_data);
    segdata->height = segdata->width = segdata->channels = 0;
  }

  LOG_DEBUG("Loading %s to memory", filename);
  segdata->img_data = stbi_load(filename, &segdata->width,
      &segdata->height, &segdata->channels, 1);
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

  if (!debug_run)
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

int
greyscale_blur(unsigned char *data, int w, int h, int box_len,
    unsigned char *hblur_data, unsigned char *blur_data)
{
  int r;
  int i, j;
  int box_avg = 0;

  if (hblur_data == NULL || blur_data == NULL) {
    LOG_ERR("Invalid hblur_data or blur_data");
    return -1;
  }

  if (box_len % 2 == 0) {
    LOG_ERR("Blur box length should be an odd number!");
    return -1;
  }
  r = box_len / 2;

  // Horizontal blur
  for (i = 0; i < h; i++) {
    int p; 
    unsigned char val;
    box_avg = 0;
    for (j = -r; j <= r; j++) {
      int k = (j < 0) ? 0 : j;
      box_avg += *(data + (i * w) + k);
    }
    box_avg /= box_len;

    for (j = 0; j < w; j++) {
      box_avg = (box_avg > MAX_PIXEL_VALUE) ? MAX_PIXEL_VALUE : box_avg;
      *(hblur_data + (i * w) + j) = box_avg;
      
      p = j - r;
      if (p < 0)
        val = *(data + (i * w) + 0);
      else
        val = *(data + (i * w) + p);

      box_avg -= (val / box_len);

      p = j + r + 1;
      if (p >= w)
        val = *(data + (i * w) + (w - 1));
      else
        val = *(data + (i * w) + p);

      box_avg += (val / box_len);
    }
  }

  // Vertical Blur
  for (i = 0; i < w; i++) {
    int p;
    unsigned char val;
    box_avg = 0;
    for (j = -r; j <= r; j++) {
      int k = (j < 0) ? 0 : j;
      box_avg += *(hblur_data + (k * w) + i);
    }
    box_avg /= box_len;

    for (j = 0; j < h; j++) {
      box_avg = (box_avg > MAX_PIXEL_VALUE) ? MAX_PIXEL_VALUE : box_avg;
      *(blur_data + (j * w) + i) = box_avg;

      p = j - r;
      if (p < 0)
        val = *(hblur_data + (0 * w) + i);
      else
        val = *(hblur_data + (p * w) + i);

      box_avg -= val / box_len;

      p = j + r + 1;
      if (p >= h)
        val = *(hblur_data + ((h - 1) * w) + i);
      else
        val = *(hblur_data + (p * w) + i);

      box_avg += val / box_len;
    }
  }

  return 0;
}

double
average_intensity(int *integrals, int x, int y, int w, int h, int winsz)
{
  int x1, x2, y1, y2;
  int a;
  int r = winsz / 2;

  x1 = x - r - 1; // x1 - 1
  x2 = x + r;
  y1 = y - r - 1; // y1 - 1
  y2 = y + r;

  if (x2 >= h)
    x2 = h - 1;

  if (y2 >= w)
    y2 = w - 1;

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;

  a = *(integrals + (x2 * w) + y2);

  if (x1 >= 0 && y1 >= 0)
    a += *(integrals + ((x1 * w) + y1));

  if (y1 >= 0)
    a -= *(integrals + ((x2 * w) + y1));

  if (x1 >= 0)
    a -= *(integrals + ((x1 * w) + y2));

  return a / (double)(winsz * winsz);
}

int
threshold(unsigned char *img_data, int w, int h,
    int x1, int y1, int x2, int y2,
    int *integrals, bool *threshold)
{
  int x, y;
  int i, j;

  if (integrals == NULL || threshold == NULL) {
    LOG_ERR("Invalid integrals or threshold!");
    return -1;
  }

  x = x2 - x1;
  y = y2 - y1;
  if (x < 0 || y < 0) {
    LOG_ERR("Invalid coordinates for threshold!");
    return -1;
  }

  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      // I(i, j) = F(i, j) + I(i - 1, j) + I (i, j - 1) - I(i - 1, j - 1)
      int i1 = 0, i2 = 0, i3 = 0;
      if (i != 0) {
        i1 = *(integrals + (((i - 1) * w) + j));
      }
      if (j != 0) {
        i2 = *(integrals + ((i * w) + (j - 1)));
      }
      if (i != 0 && j != 0) {
        i3 = *(integrals + (((i - 1) * w) + (j - 1)));
      }
      *(integrals + ((i * w) + j)) = *(img_data + ((i * w) + j)) + i1 + i2 - i3;
    }
  }

  bzero(threshold, sizeof(bool) * w * h);
  for (i = x1; i < x2; i++) {
    for (j = y1; j < y2; j++) {
      unsigned char pixel = *(img_data + ((i * y) + j));
      double avg = average_intensity(integrals, i, j, w, h, threshold_neighbor_sz);
      if ((pixel / avg) < threshold_ratio) {
        *(threshold + ((i - x1) * y) + (j - y1)) = true;
      } else {
        *(threshold + ((i - x1) * y) + (j - y1)) = false;
      }
    }
  }

  return 0;
}

int segdata_process(segdata_t *segdata)
{
  int w, h;
  connected_component_t largest;

  if (segdata == NULL) {
    LOG_ERR("Inavlid segdata!");
    return -1;
  }
  w = segdata->width;
  h = segdata->height;
  LOG_DEBUG("Processing Image %s", segdata->filename);
  if (greyscale_blur(segdata->img_data, w, h, 3,
        segdata->hblur_data, segdata->blur_data) < 0) {
    LOG_ERR("Failed to blur the image");
    return -1;
  }
  if (debug_run) {
    stbi_write_jpg(segdata->blur_filename, segdata->width,
        segdata->height, 1, segdata->blur_data, 0);
  }

  if (threshold(segdata->blur_data, w, h, 0, 0, h, w,
        segdata->integral_data, segdata->threshold_data) < 0) {
    LOG_ERR("hresholding failed!");
    return -1;
  }
  if (debug_run) {
    write_threshold(segdata->threshold_filename,
        segdata->threshold_data, segdata->tmp_data, w, h);
  }
  largest = largest_component(segdata->threshold_data, w, h);
  printf("%d %d %d\n",
      largest.total_x / largest.count,
      largest.total_y / largest.count,
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
