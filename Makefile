RM = rm -rf
MKDIR = mkdir -p

INC_DIR= inc
SRC_DIR= src
BLD_DIR = build
OBJ_DIR = $(BLD_DIR)/objs
DEPS_DIR = $(BLD_DIR)/deps
BIN_DIR = $(BLD_DIR)/bin
LIB_DIR = $(BLD_DIR)/lib

SEGMENTER_OBJ_DIR = $(OBJ_DIR)/segmenter
SEGMENTER_DEPS_DIR = $(DEPS_DIR)/segmenter
SEGMENTER = $(BIN_DIR)/segmenter

SEGMENT_OBJ_DIR = $(OBJ_DIR)/segment
SEGMENT_DEPS_DIR = $(DEPS_DIR)/segment
SEGMENT = $(BIN_DIR)/segment

SEGMENTLIB_OBJ_DIR = $(OBJ_DIR)/segmentlib
SEGMENTLIB_DEPS_DIR = $(DEPS_DIR)/segmentlib
SEGMENTLIB = $(LIB_DIR)/segmentlib.so

OMPI_OBJ_DIR = $(OBJ_DIR)/mpi
OMPI_DEPS_DIR = $(DEPS_DIR)/mpi
MPISEGMENTER = $(BIN_DIR)/msegmenter

CC=gcc
MPICC=mpicc
ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -g
else
  CFLAGS += -Ofast
endif

CFLAGS += -std=gnu99
CFLAGS += -Wall -Werror
CFLAGS += -I$(INC_DIR)

ifeq ($(PROFILE), 1)
	CFLAGS += -DPROFILE_SEGMENTER
endif

LDLIBS = -lm -lpthread

SEGMENT_OBJS =	\
							segmentmain.o

SEGMENTLIB_OBJS = \
							segmentlib.o

MAIN_OBJS =       \
            main.o

MPIMAIN_OBJS =          \
               mpimain.o

SEGMENTER_OBJS =                    \
                 argparser.o        \
                 largestcomponent.o \
                 log.o              \
                 profile.o          \
                 segmenter.o        \
                 segmentation.o			\
								 work_queue.o

.PHONY: all

segmenter: $(SEGMENTER)

segment : $(SEGMENT)

segmentlib : $(SEGMENTLIB)

mpi: $(MPISEGMENTER)

all: segmenter mpi segment segmentlib

$(SEGMENTER_OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(SEGMENTER_OBJ_DIR) $(SEGMENTER_DEPS_DIR)
	$(CC) $(CFLAGS) -MMD -MF $(SEGMENTER_DEPS_DIR)/$(*F).d -c -o $@ $<

$(SEGMENT_OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(SEGMENT_OBJ_DIR) $(SEGMENT_DEPS_DIR)
	$(CC) $(CFLAGS) -MMD -MF $(SEGMENT_DEPS_DIR)/$(*F).d  -DSINGLE_FRAME -c -o $@ $<

$(SEGMENTLIB_OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(SEGMENTLIB_OBJ_DIR) $(SEGMENTLIB_DEPS_DIR)
	$(CC) $(CFLAGS) -fPIC -MMD -MF $(SEGMENTLIB_DEPS_DIR)/$(*F).d  -DSINGLE_FRAME -c -o $@ $<

$(OMPI_OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(OMPI_OBJ_DIR) $(OMPI_DEPS_DIR)
	$(MPICC) $(CFLAGS) -MMD -MF $(OMPI_DEPS_DIR)/$(*F).d -c -o $@ $<

$(BIN_DIR) $(LIB_DIR) $(SEGMENTER_OBJ_DIR) $(SEGMENTER_DEPS_DIR) $(OMPI_OBJ_DIR) $(OMPI_DEPS_DIR) \
	$(SEGMENTLIB_OBJ_DIR) $(SEGMENTLIB_DEPS_DIR) $(SEGMENT_OBJ_DIR) $(SEGMENT_DEPS_DIR):
		$(MKDIR) $@

$(SEGMENT): $(SEGMENT_OBJS:%.o=$(SEGMENT_OBJ_DIR)/%.o) $(SEGMENTER_OBJS:%.o=$(SEGMENT_OBJ_DIR)/%.o) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(SEGMENTLIB): $(SEGMENTLIB_OBJS:%.o=$(SEGMENTLIB_OBJ_DIR)/%.o) $(SEGMENTER_OBJS:%.o=$(SEGMENTLIB_OBJ_DIR)/%.o) | $(LIB_DIR)
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(SEGMENTER): $(SEGMENTER_OBJS:%.o=$(SEGMENTER_OBJ_DIR)/%.o) $(MAIN_OBJS:%.o=$(SEGMENTER_OBJ_DIR)/%.o) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(MPISEGMENTER): $(MPIMAIN_OBJS:%.o=$(OMPI_OBJ_DIR)/%.o) $(SEGMENTER_OBJS:%.o=$(OMPI_OBJ_DIR)/%.o) | $(BIN_DIR)
	$(MPICC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: clean

clean:
	$(RM) $(BLD_DIR)

DEPS_FILES = $(SEGMENTER_OBJS:%.o=$(DEPS_DIR)/%.d)
SEGMENT_DEPS_FILES = $(SEGMENTER_OBJS:%.o=$(SEGMENT_DEPS_DIR)/%.d)
OMPI_DEPS_FILES = $(SEGMENTER_OBJS:%.o=$(OMPI_DEPS_DIR)/%.d)

-include $(DEPS_FILES)

