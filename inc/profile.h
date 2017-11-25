#pragma once


#define NANOSECOND  1000000000ULL

unsigned long long get_ns();

#ifdef PROFILE_SEGMENTER
#define DEFINE_PROFILE_VARS unsigned long long start, end;

#define START_PROFILE \
{                     \
  start = get_ns(); \
}                   \

#define END_PROFILE(func) \
{                   \
  end = get_ns();   \
  LOG_DEBUG("PROFILE: %s time: %llu", func, end - start); \
}                   \

#else

#define DEFINE_PROFILE_VARS
#define START_PROFILE
#define END_PROFILE(func)

#endif
