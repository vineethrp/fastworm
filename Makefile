RM = rm -rf
MKDIR = mkdir -p

INC_DIR= inc
SRC_DIR= src
BLD_DIR = build
OBJ_DIR = $(BLD_DIR)/objs
DEPS_DIR = $(BLD_DIR)/deps
BIN_DIR = $(BLD_DIR)/bin
SEGMENTER = $(BIN_DIR)/segmenter

CC=gcc
CFLAGS += -g -O0
CFLAGS += -std=gnu99
CFLAGS += -Wall -Werror
CFLAGS += -I$(INC_DIR)

LDLIBS = -lm

SEGMENTER_OBJS = 								\
								 log.o 					\
								 main.o					\
								 segmentation.o

.PHONY: all

all: $(SEGMENTER)


$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(OBJ_DIR) $(DEPS_DIR)
	$(CC) $(CFLAGS) -MMD -MF $(DEPS_DIR)/$(*F).d -c -o $@ $<

$(OBJ_DIR) $(DEPS_DIR) $(BIN_DIR):
		$(MKDIR) $@

$(SEGMENTER): $(SEGMENTER_OBJS:%.o=$(OBJ_DIR)/%.o) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: clean

clean:
	$(RM) $(BLD_DIR)

DEPS_FILES = $(SEGMENTER_OBJS:%.o=$(DEPS_DIR)/%.d)

-include $(DEPS_FILES)

