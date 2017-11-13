#ifndef _SEGMENTER_LOG_H_
#define _SEGMENTER_LOG_H_

#define LOGBUF_SZ 10485760 // 10MB
#define LOG_LINE_MAX  1024
#define TIME_STR_LEN  64

typedef enum {
  LOG_ERR = 0,
  LOG_WARN = 1,
  LOG_INFO = 2,
  LOG_DEBUG = 3,
  LOG_X_DEBUG = 4,
  LOG_XX_DEBUG = 5
} loglevel_t;

int log_init(loglevel_t, unsigned, const char *);
void log_fini();

int log_write(loglevel_t, const char *, ...);
int log_flush();


#define LOG_ERR(format, ...) log_write(LOG_ERR, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) log_write(LOG_WARN, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) log_write(LOG_INFO, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) log_write(LOG_DEBUG, format, ##__VA_ARGS__)
#define LOG_X_DEBUG(format, ...) log_write(LOG_X_DEBUG, format, ##__VA_ARGS__)
#define LOG_XX_DEBUG(format, ...) log_write(LOG_XX_DEBUG, format, ##__VA_ARGS__)

#endif // _SEGMENTER_LOG_H_
