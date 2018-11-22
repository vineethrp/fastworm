%module segment
%{
        #include <stdio.h>
        #include <stdlib.h>
        #include <stdbool.h>
        #include <string.h>
        #include <limits.h>
        #include <unistd.h>
        #include <errno.h>
        #include <sys/stat.h>
        #include <sys/types.h>

        #include <pthread.h>

        #include "log.h"
        #include "argparser.h"
        #include "work_queue.h"
        #include "segmenter.h"
        #include "segmentlib.h"
%}
%include "segmenter.h"
%include "segmentlib.h"
