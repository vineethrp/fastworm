#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/syscall.h>

#include "log.h"

extern char *program_invocation_short_name;

static const char *loglevel_str[] = {
  "LOG_ERROR",
  "LOG_WARNING",
  "LOG_INFO",
  "LOG_DEBUG",
  "LOG_X_DEBUG",
  "LOG_XX_DEBUG"
};

static loglevel_t desired_level = LOG_INFO;
static unsigned long logbuf_sz = LOGBUF_SZ;
static char *logbuf = NULL;
static char *logbuf_ptr = NULL;
static int log_fd = -1;
static bool log_rotated = false;

int
log_init(loglevel_t desired_loglevel, unsigned logbuf_size, const char *logfile)
{
  if (desired_loglevel > LOG_INFO)
    desired_level = desired_loglevel;

	if (logbuf_size != 0)
		logbuf_sz = logbuf_size;
  logbuf = (char *) malloc(logbuf_sz);
  if (logbuf == NULL) {
    fprintf(stderr, "Failed to allocate the log buffer!\n");
    return -1;
  }
  logbuf_ptr = logbuf;

  log_fd = open(logfile, O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (log_fd < 0) {
    fprintf(stderr, "Failed to open log file!\n");
    return -1;
  }

  return 0;
}

void
log_fini()
{
  log_flush();

  if (log_fd > 0)
    close(log_fd);
  if (logbuf)
    free(logbuf);
}

static int
get_current_asciitime(char *time_buf)
{
	struct timeval tv;
	time_t nowtime;
	struct tm *nowtm;
	char buf[TIME_STR_LEN - 7];

	if (buf == NULL)
		return -1;

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	nowtm = localtime(&nowtime);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", nowtm);
	snprintf(time_buf, TIME_STR_LEN, "%s.%.6d", buf, (int)tv.tv_usec);

	return 0;
}

int
log_write(loglevel_t loglevel, const char *format, ...)
{
	int ret = 0;
  va_list ap;
	char time_buf[TIME_STR_LEN];
  char buf[LOG_LINE_MAX];
  char *buf_ptr = buf;

  if (logbuf == NULL)
    return 0;

  if (loglevel > desired_level)
    return 0;

	if (loglevel > LOG_XX_DEBUG)
		loglevel = LOG_XX_DEBUG;

	get_current_asciitime(time_buf);
	ret = sprintf(buf_ptr,
			"%s %s(%d:%ld) [%s]: ",
			time_buf,
			program_invocation_short_name,
			getpid(),
			syscall(SYS_gettid),
			loglevel_str[loglevel]);
  buf_ptr = buf_ptr + ret;


  va_start(ap, format);
  ret += vsprintf(buf_ptr, format, ap);
  va_end(ap);

  buf_ptr = buf + ret;

  if (ret >= LOG_LINE_MAX) {
    buf_ptr = buf + LOG_LINE_MAX - 2;
  }

  buf_ptr[0] = '\n';
  //buf_ptr[1] = '\0';
  ret += 1;

  if ((uintptr_t)(logbuf_ptr - logbuf + ret) > logbuf_sz) {
    log_rotated = true;
    logbuf_ptr = logbuf;
  }
  logbuf_ptr += ret;
  memcpy(logbuf_ptr - ret, buf, ret);

  return ret;
}

int
log_flush()
{
  int ret = 0;
  size_t count = logbuf_ptr - logbuf;;
  char *logbuf_start = logbuf;

  if (log_fd < 0) {
    fprintf(stderr, "Log file not opened!\n");
    return -1;
  }

  if (log_rotated) {
    logbuf_start = logbuf_ptr;
    ret = write(log_fd, logbuf_start, logbuf_sz - count);
    logbuf_start = logbuf;
    log_rotated = false;
  }

  ret += write(log_fd, logbuf_start, count);

  return ret;
}
