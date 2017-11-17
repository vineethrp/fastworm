#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdbool.h>

#include "segmentation.h"
#include "log.h"
#include "largestcomponent.h"

static int threshold_neighbor_sz = 25;
static float threshold_ratio = 0.9;

static int write_threshold(bool *threashold_data, int w, int h);

unsigned char *
greyscale_blur(unsigned char *data, int w, int h, int box_len)
{
  int r;
  int i, j;
  int box_avg = 0;
  unsigned char *hblur_data = NULL;
  unsigned char *blur_data = NULL;

  if (box_len % 2 == 0) {
    LOG_ERR("Blur box length should be an odd number!");
    return NULL;
  }
  r = box_len / 2;

  hblur_data = (unsigned char *) malloc(w * h);
  if (hblur_data == NULL) {
    LOG_ERR("Failed to malloc horizontal blur buffer!");
    return NULL;
  }

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

  blur_data = (unsigned char *) malloc(w * h);
  if (blur_data == NULL) {
    LOG_ERR("Failed to malloc blur buffer!");
    return NULL;
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

  free(hblur_data);
  return blur_data;
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

bool *
threshold(unsigned char *img_data, int w, int h, int x1, int y1, int x2, int y2)
{
  int x, y;
  int i, j;
  int *integrals = NULL;
  bool *threshold = NULL;

  x = x2 - x1;
  y = y2 - y1;
  if (x < 0 || y < 0)
    return NULL;

  integrals = (int *)calloc(w * h, sizeof(int));
  if (integrals == NULL) {
    LOG_ERR("Failed to allocate integral array!");
    return NULL;
  }

  threshold = (bool *) calloc(x * y, sizeof(bool));
  if (threshold == NULL) {
    LOG_ERR("Failed to allocate threshold array!");
    free(integrals);
    return NULL;
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

  free(integrals);
  return threshold;
}

int
find_centroid(char *filename, worm_data_t *worm_data)
{
  int w, h, n;
  unsigned char *img_data = NULL;
  unsigned char *blur_data = NULL;
  bool *threshold_data = NULL;
  connected_component_t largest;

  LOG_DEBUG("Segmenting %s", filename);
  stbi_info(filename, &w, &h, &n);
  if (stbi_failure_reason() != NULL) {
    LOG_ERR("Failed to get information for image: %s\n", filename);
    return -1;
  }
  LOG_DEBUG("Image details(%s): w=%d, h=%d, n=%d",
      filename, w, h, n);

  img_data = stbi_load(filename, &w, &h, &n, 1);
  if (img_data == NULL) {
    LOG_ERR("Failed to load the image to memory!");
    return -1;
  }
  stbi_write_png("output.png", w, h, 1, img_data, w);
  blur_data = greyscale_blur(img_data, w, h, 3);
  if (blur_data == NULL) {
    LOG_ERR("blur failed!");
    return -1;
  }
  stbi_write_jpg("output_blur.jpg", w, h, 1, blur_data, 0);

  threshold_data = threshold(blur_data, w, h, 0, 0, h, w);
  write_threshold(threshold_data, w, h);
  largest = largest_component(threshold_data, w, h);
  printf("%d %d %d\n",
      largest.total_x / largest.count,
      largest.total_y / largest.count,
      largest.count);

  return 0;
}

static int
write_threshold(bool *threshold_data, int w, int h)
{
  int i, j;
  unsigned char *data = NULL;

  data = (unsigned char *)malloc(w * h);
  if (data == NULL) {
    LOG_ERR("Failed to allocate threshold write buffer!");
    return -1;
  }

  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      if (*(threshold_data + (i * w) + j) == true) {
        *(data + (i * w) + j) = 255;
      } else {
        *(data + (i * w) + j) = 0;
      }
    }
  }
  stbi_write_jpg("output_threshold.jpg", w, h, 1, data, 0);

  free(data);
  return 0;
}
