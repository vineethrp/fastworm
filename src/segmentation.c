#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <strings.h>

#include "segmenter.h"
#include "log.h"
#include "segmentation.h"

int
greyscale_blur(unsigned char *data, int w, int h,
    int x1, int y1, int x2, int y2, int box_len,
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
  for (i = x1; i < x2; i++) {
    int p; 

    for (j = y1-r; j <= y1+r; j++) {
      int k = (j < 0) ? 0 : j;
      box_avg += *(data + (i * w) + k);
    }
    box_avg /= box_len;

    for (j = y1; j < y2; j++) {
      box_avg = (box_avg > MAX_PIXEL_VALUE) ? MAX_PIXEL_VALUE : box_avg;
      *(hblur_data + (i * w) + j) = box_avg;
      
      p = j - r;
      if (p < 0)
        p = 0;

      box_avg -= (*(data + (i * w) + p) / box_len);

      p = j + r + 1;
      if (p >= w)
        p = w - 1;

      box_avg += (*(data + (i * w) + p) / box_len);
    }
  }

  // Vertical Blur
  for (i = y1; i < y2; i++) {
    int p;
    box_avg = 0;
    for (j = x1-r; j <= x1+r; j++) {
      int k = (j < 0) ? 0 : j;
      box_avg += *(hblur_data + (k * w) + i);
    }
    box_avg /= box_len;

    for (j = x1; j < x2; j++) {
      box_avg = (box_avg > MAX_PIXEL_VALUE) ? MAX_PIXEL_VALUE : box_avg;
      *(blur_data + (j * w) + i) = box_avg;

      p = j - r;
      if (p < 0)
        p = 0;

      box_avg -= *(hblur_data + (p * w) + i) / box_len;

      p = j + r + 1;
      if (p >= h)
        p = h - 1;

      box_avg += *(hblur_data + (p * w) + i) / box_len;
    }
  }

  return 0;
}

static inline int
box_sum(unsigned char *data, int w, int h,
    int x, int y)
{
  int sum = 0;
  for (int i = x - 1; i <= x + 1; i++) {
    for (int j = y - 1; j <= y + 1; j++) {
      int x1 = i, y1 = j;
      if (x1 < 0) x1 = 0;
      if (x1 >= h) x1 = h - 1;
      if (y1 < 0) y1 = 0;
      if (y1 >= w) y1 = w - 1;
      sum += *(data + (x1 * w) + y1);
    }
  }

  return sum;
}

int
greyscale_simple_blur(unsigned char *data, int w, int h,
    int x1, int y1, int x2, int y2, int box_len,
    unsigned char *hblur_data, unsigned char *blur_data)
{
  if (box_len > 3)
    greyscale_blur(data, w, h, x1, y1, x2, y2, box_len,
        hblur_data, blur_data);

  for (int i = x1; i < x2; i++) {
    for (int j = y1; j < y2; j++) {
      int avg = box_sum(data, w, h, i, j) / 9;
      if (avg > MAX_PIXEL_VALUE) avg = MAX_PIXEL_VALUE;
      *(blur_data + (i * w) + j) = avg;
    }
  }

  return 0;
}

static double
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
dynamic_threshold(unsigned char *img_data, int w, int h,
    int x1, int y1, int x2, int y2,
    int threshold_winsz, float threshold_ratio,
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

  bzero(integrals, sizeof(int) * w * h);
  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      // I(i, j) = F(i, j) + I(i - 1, j) + I (i, j - 1) - I(i - 1, j - 1)
      int i1 = 0, i2 = 0, i3 = 0;
      if (i != 0) {
        i1 = *(integrals + (((i - 1) * w) + j));
      }
      if (j != 0) {
        i2 = *(integrals + (i * w) + (j - 1));
      }
      if (i != 0 && j != 0) {
        i3 = *(integrals + (((i - 1) * w) + (j - 1)));
      }
      *(integrals + (i * w) + j) = *(img_data + (i * w) +j) + i1 + i2 - i3;
    }
  }

  bzero(threshold, sizeof(bool) * w * h);
  for (i = x1; i < x2; i++) {
    for (j = y1; j < y2; j++) {
      unsigned char pixel = *(img_data + ((i * w) + j));
      double avg = average_intensity(integrals, i, j, w, h, threshold_winsz);
      if ((pixel / avg) < threshold_ratio) {
        //*(threshold + ((i - x1) * y) + (j - y1)) = true;
        *(threshold + (i * w) + j) = true;
      }
    }
  }

  return 0;
}

int
simple_threshold(unsigned char *img_data, int w, int h,
    int x1, int y1, int x2, int y2,
    int threshold_winsz, float threshold_ratio,
    int *integrals, bool *threshold)
{
  unsigned char threshold_pixel = 255 * threshold_ratio;

  for (int i = x1; i < x2; i++) {
    for (int j = y1; j < y2; j++) {
      if (*(img_data + (i * w) + j) < threshold_pixel)
        *(threshold + (i * w) + j) = true;
    }
  }
  return 0;
}

