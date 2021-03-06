#include <stdio.h>
#include <stdbool.h>

#include "log.h"
#include "largestcomponent.h"

static void
find_connected_component(bool *threshold_data,
    int x, int y, int w, int h, connected_component_t *c)
{
  if (c == NULL)
    return;

  if (*(threshold_data + (x * w) + y) == false)
    return;

  *(threshold_data + (x * w) + y) = false;

  c->total_x += x;
  c->total_y += y;
  c->count++;

  for (int i = x - 1; i <= x + 1; i++) {
    if (i < 0 || i >= h) continue;
    for (int j = y - 1; j <= y + 1; j++) {
      if (j < 0 || j >= w) continue;
      if (*(threshold_data + (i * w) + j)) {
        find_connected_component(threshold_data, i, j, w, h, c);
      }
    }
  }

}

connected_component_t
largest_component(bool *threshold_data, int x1, int y1, int x2, int y2, int w, int h)
{
  connected_component_t largest_component = { 0 };

  for (int i = x1; i < x2; i++) {
    for (int j = y1; j < y2; j++) {
      if (*(threshold_data + (i * w) + j)) {
        connected_component_t c = { 0 };
        find_connected_component(threshold_data, i, j, w, h, &c);
        LOG_DEBUG("Component: %d %d %d", c.total_x, c.total_y, c.count);
        if (c.count > largest_component.count)
          largest_component = c;
      }
    }
  }

  return largest_component;
}
