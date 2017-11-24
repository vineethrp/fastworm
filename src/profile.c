#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "log.h"
#include "profile.h"


unsigned long long
get_ns()
{
  struct timespec ts = { 0 };
  if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
    LOG_ERR("clock_gettime failed");
    return 0;
  }
  return ts.tv_sec * NANOSECOND + ts.tv_nsec;
}

