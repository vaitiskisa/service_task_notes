.PHONY: clean unit-test coverage coverage-clean

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
CFLAGS += -I$(SRC_PATH)/utils
CFLAGS += -I$(SRC_PATH)/notes
CFLAGS += -I$(SRC_PATH)/notes_repository
CFLAGS += -I$(SRC_PATH)/service
CFLAGS += -I$(SRC_PATH)/notes_handler

CSRCS :=
include $(SRC_PATH)/http/http.mk
include $(SRC_PATH)/utils/utils.mk
include $(SRC_PATH)/notes/notes.mk
include $(SRC_PATH)/notes_repository/notes_repository.mk
include $(SRC_PATH)/service/service.mk
include $(SRC_PATH)/notes_handler/notes_handler.mk

CSRCS := $(CSRCS) $(SRC_PATH)/main.c
DEPS  := $(shell find $(BUILD_PATH) -type f -name '*.d' -print)

OBJS := $(OBJS) $(patsubst %.c,$(BUILD_PATH)/%.o, $(CSRCS))

$(BUILD_TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(BUILD_TARGET) $(LDFLAGS) $(SANITIZER_BIN)

-include $(DEPS)

$(BUILD_PATH)/%.o: $(ROOT_PATH)/%.c
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

UNIT_TEST_CFLAGS := $(filter-out -c -MD,$(CFLAGS))
UNIT_TEST_LDFLAGS := $(LDFLAGS) -lcmocka
UNIT_TEST_SRCS :=
include $(SRC_PATH)/http/unittests/http_test.mk
include $(SRC_PATH)/utils/unittests/utils_tests.mk
include $(SRC_PATH)/notes/unittests/notes_tests.mk
include $(SRC_PATH)/notes_repository/unittests/notes_repository_test.mk
include $(SRC_PATH)/service/unittests/service_tests.mk
include $(SRC_PATH)/notes_handler/unittests/notes_handler_tests.mk

$(UNIT_TEST_HTTP_TARGET): $(UNIT_TEST_HTTP_SRCS)
	@ mkdir -p $(dir $@)
	$(CC) $(UNIT_TEST_CFLAGS) $(UNIT_TEST_HTTP_CFLAGS) $(UNIT_TEST_HTTP_SRCS) -o $(UNIT_TEST_HTTP_TARGET) $(UNIT_TEST_LDFLAGS) $(SANITIZER_BIN)

$(UNIT_TEST_NOTES_TARGET): $(UNIT_TEST_NOTES_SRCS)
	@ mkdir -p $(dir $@)
	$(CC) $(UNIT_TEST_CFLAGS) $(UNIT_TEST_NOTES_CFLAGS) $(UNIT_TEST_NOTES_SRCS) -o $(UNIT_TEST_NOTES_TARGET) $(UNIT_TEST_LDFLAGS) $(SANITIZER_BIN)

$(UNIT_TEST_NOTES_HANDLER_TARGET): $(UNIT_TEST_NOTES_HANDLER_SRCS)
	@ mkdir -p $(dir $@)
	$(CC) $(UNIT_TEST_CFLAGS) $(UNIT_TEST_NOTES_HANDLER_CFLAGS) $(UNIT_TEST_NOTES_HANDLER_SRCS) -o $(UNIT_TEST_NOTES_HANDLER_TARGET) $(UNIT_TEST_LDFLAGS) $(SANITIZER_BIN)

$(UNIT_TEST_NOTES_REPO_TARGET): $(UNIT_TEST_NOTES_REPO_SRCS)
	@ mkdir -p $(dir $@)
	$(CC) $(UNIT_TEST_CFLAGS) $(UNIT_TEST_NOTES_REPO_CFLAGS) $(UNIT_TEST_NOTES_REPO_SRCS) -o $(UNIT_TEST_NOTES_REPO_TARGET) $(UNIT_TEST_LDFLAGS) $(SANITIZER_BIN)

$(UNIT_TEST_NOTES_SERVICE_TARGET): $(UNIT_TEST_NOTES_SERVICE_SRCS)
	@ mkdir -p $(dir $@)
	$(CC) $(UNIT_TEST_CFLAGS) $(UNIT_TEST_NOTES_SERVICE_CFLAGS) $(UNIT_TEST_NOTES_SERVICE_SRCS) -o $(UNIT_TEST_NOTES_SERVICE_TARGET) $(UNIT_TEST_LDFLAGS) $(SANITIZER_BIN)

$(UNIT_TEST_UTILS_TARGET): $(UNIT_TEST_UTILS_SRCS)
	@ mkdir -p $(dir $@)
	$(CC) $(UNIT_TEST_CFLAGS) $(UNIT_TEST_UTILS_CFLAGS) $(UNIT_TEST_UTILS_SRCS) -o $(UNIT_TEST_UTILS_TARGET) $(UNIT_TEST_LDFLAGS) $(SANITIZER_BIN)

COVERAGE_CFLAGS := $(filter-out -fsanitize=leak -fsanitize=address -fsanitize=undefined,$(CFLAGS)) --coverage -O0
COVERAGE_LDFLAGS := $(filter-out -lasan,$(LDFLAGS)) --coverage
COVERAGE_INFO := $(BUILD_PATH)/coverage/coverage.info
COVERAGE_HTML := $(BUILD_PATH)/coverage/html

unit-test: $(UNIT_TEST_HTTP_TARGET) $(UNIT_TEST_NOTES_TARGET) $(UNIT_TEST_NOTES_HANDLER_TARGET) $(UNIT_TEST_NOTES_REPO_TARGET) $(UNIT_TEST_NOTES_SERVICE_TARGET) $(UNIT_TEST_UTILS_TARGET)
	$(UNIT_TEST_HTTP_TARGET)
	$(UNIT_TEST_NOTES_TARGET)
	$(UNIT_TEST_NOTES_HANDLER_TARGET)
	$(UNIT_TEST_NOTES_REPO_TARGET)
	$(UNIT_TEST_NOTES_SERVICE_TARGET)
	$(UNIT_TEST_UTILS_TARGET)

coverage-clean:
	find . -name '*.gcda' -o -name '*.gcno' | xargs -r rm -f
	rm -rf $(BUILD_PATH)/coverage

coverage: coverage-clean
	$(MAKE) clean
	$(MAKE) unit-test CFLAGS="$(COVERAGE_CFLAGS)" LDFLAGS="$(COVERAGE_LDFLAGS)"
	@ mkdir -p $(dir $(COVERAGE_INFO))
	lcov --capture --directory . --output-file $(COVERAGE_INFO)
	lcov --remove $(COVERAGE_INFO) '/usr/*' '*/unittests/*' --output-file $(COVERAGE_INFO)
	genhtml $(COVERAGE_INFO) --output-directory $(COVERAGE_HTML)

clean:
	rm -rf $(BUILD_PATH)
