#pragma once

typedef struct connected_component_s {
  int total_x;
  int total_y;
  int count;
} connected_component_t;

connected_component_t largest_component(bool *threshold_data,
    int x1, int y1, int x2, int y2, int w, int h);

