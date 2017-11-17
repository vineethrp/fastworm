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

  if ((x - 1) >= 0 && *(threshold_data + ((x - 1) * w) + y))
    find_connected_component(threshold_data, x - 1, y, w, h, c);

  if ((x + 1) < h && *(threshold_data + ((x + 1) * w) + y))
    find_connected_component(threshold_data, x + 1, y, w, h, c);

  if ((y - 1) >= 0 && *(threshold_data + (x * w) + (y - 1)))
    find_connected_component(threshold_data, x, y - 1, w, h, c);

  if ((y + 1) < w && *(threshold_data + (x * w) + (y + 1)))
    find_connected_component(threshold_data, x, y + 1, w, h, c);

}

connected_component_t
largest_component(bool *threshold_data, int w, int h)
{
  int i, j;
  connected_component_t largest_component = { 0 };

  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      if (*(threshold_data + (i * w) + j) == 1) {
        connected_component_t c = { 0 };
        find_connected_component(threshold_data, i, j, w, h, &c);
        printf("Component: %d %d %d\n", c.total_x, c.total_y, c.count);
        if (c.count > largest_component.count)
          largest_component = c;
      }
    }
  }

  return largest_component;
}
