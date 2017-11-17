#pragma once

#define MAX_PIXEL_VALUE 255
typedef struct worm_data_s {
  float centroid_x;
  float centroid_y;
  float worm_area;
} worm_data_t;

int find_centroid(char *filename, worm_data_t *worm_data);
