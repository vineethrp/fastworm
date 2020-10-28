#pragma once


#define NANOSECOND  1000000000ULL

unsigned long long get_ns();

#ifdef PROFILE_SEGMENTER

typedef enum {
  PROFILE_IMGLOAD = 0,
  PROFILE_IMGBLUR,
  PROFILE_THRESHOLD,
  PROFILE_LC,	/* Largest Component */
  PROFILE_MAX
} profile_type_t;

typedef struct {
  unsigned long long data[PROFILE_MAX];
} profile_data_t;


extern char *profile_data_strs[];

#define DEFINE_PROFILE_VARS unsigned long long start, end;

#define START_PROFILE \
{                     \
  start = get_ns(); \
}                   \

#define END_PROFILE(pd_type, pd) \
{                   \
  end = get_ns();   \
  pd.data[pd_type] = end - start;	\
  LOG_DEBUG("PROFILE: %s time: %llu", profile_data_strs[pd_type], pd.data[pd_type]); \
}                   \

#else

#define DEFINE_PROFILE_VARS
#define START_PROFILE
#define END_PROFILE(pd_type, pd)

#endif
