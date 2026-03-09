.PHONY: clean




ROOT_PATH := .
SRC_PATH := $(ROOT_PATH)/src
BUILD_PATH := $(ROOT_PATH)/build

ifneq ($(wildcard /usr/lib/clang),)
    ifneq ($(wildcard /usr/bin/clang),)
		CLANG_VERSION := $(shell clang --version | sed -n 's/^clang version \([0-9.]*\).*/\1/p' | cut -d. -f1)
		CLANG_DIR := /usr/lib/clang/$(CLANG_VERSION)/lib/linux

		ifneq ($(wildcard $(CLANG_DIR)),)
			CFLAGS := $(CFLAGS) -fsanitize=leak -fsanitize=address # -fsanitize=undefined
			SANITIZER_BIN := $(CLANG_DIR)/libclang_rt.asan-x86_64.a $(CLANG_DIR)/libclang_rt.ubsan_standalone_cxx-x86_64.a
			CLANG_SANITIZER := 1
		endif
    endif
endif

ifndef CLANG_SANITIZER
	CFLAGS := $(CFLAGS) -fsanitize=leak -fsanitize=address
	LDFLAGS := $(LDFLAGS) -lasan
endif

BUILD_TARGET := $(BUILD_PATH)/service_task_notes

CFLAGS := $(CFLAGS) -c -g -MD -Wall -I/usr/include
LDFLAGS := $(LDFLAGS) -lm -lpthread -ljansson -lmicrohttpd

CFLAGS += -I/usr/include
CFLAGS += -I$(SRC_PATH)/http
CFLAGS += -I$(SRC_PATH)/global
CFLAGS += -I$(SRC_PATH)/notes
CFLAGS += -I$(SRC_PATH)/notes_repository

CSRCS :=
include $(SRC_PATH)/http/http.mk
include $(SRC_PATH)/global/global.mk
include $(SRC_PATH)/notes/notes.mk
include $(SRC_PATH)/notes_repository/notes_repository.mk

CSRCS := $(CSRCS) $(SRC_PATH)/main.c
DEPS  := $(shell find $(BUILD_PATH) -type f -name '*.d' -print)

OBJS := $(OBJS) $(patsubst %.c,$(BUILD_PATH)/%.o, $(CSRCS))

$(BUILD_TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(BUILD_TARGET) $(LDFLAGS) $(SANITIZER_BIN)

-include $(DEPS)

$(BUILD_PATH)/%.o: $(ROOT_PATH)/%.c
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_PATH)
