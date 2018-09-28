RM = rm -rf
MKDIR = mkdir -p

INC_DIR= inc
SRC_DIR= src
BLD_DIR = build
OBJ_DIR = $(BLD_DIR)/objs
DEPS_DIR = $(BLD_DIR)/deps
BIN_DIR = $(BLD_DIR)/bin
SEGMENTER = $(BIN_DIR)/segmenter

SEGMENT_OBJ_DIR = $(OBJ_DIR)/segment
SEGMENT_DEPS_DIR = $(DEPS_DIR)/segment
SEGMENT = $(BIN_DIR)/segment

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

all: $(SEGMENTER) mpi segment

segment : $(SEGMENT)

mpi: $(MPISEGMENTER)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(OBJ_DIR) $(DEPS_DIR)
	$(CC) $(CFLAGS) -MMD -MF $(DEPS_DIR)/$(*F).d -c -o $@ $<

$(SEGMENT_OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(SEGMENT_OBJ_DIR) $(SEGMENT_DEPS_DIR)
	$(CC) $(CFLAGS) -MMD -MF $(SEGMENT_DEPS_DIR)/$(*F).d  -DSINGLE_FRAME -c -o $@ $<

$(OMPI_OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(OMPI_OBJ_DIR) $(OMPI_DEPS_DIR)
	$(MPICC) $(CFLAGS) -MMD -MF $(OMPI_DEPS_DIR)/$(*F).d -c -o $@ $<

$(OBJ_DIR) $(DEPS_DIR) $(BIN_DIR) $(OMPI_OBJ_DIR) $(OMPI_DEPS_DIR) $(SEGMENT_OBJ_DIR) $(SEGMENT_DEPS_DIR):
		$(MKDIR) $@

$(SEGMENT): $(SEGMENT_OBJS:%.o=$(SEGMENT_OBJ_DIR)/%.o) $(SEGMENTER_OBJS:%.o=$(SEGMENT_OBJ_DIR)/%.o) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(SEGMENTER): $(SEGMENTER_OBJS:%.o=$(OBJ_DIR)/%.o) $(MAIN_OBJS:%.o=$(OBJ_DIR)/%.o) | $(BIN_DIR)
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

