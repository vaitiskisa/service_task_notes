UNIT_TEST_UTILS_TARGET := $(BUILD_PATH)/utils_cmocka_tests

UNIT_TEST_UTILS_SRCS += $(shell find $(SRC_PATH)/utils/unittests -type f -name '*.c' -print)
UNIT_TEST_UTILS_SRCS += \
	$(SRC_PATH)/utils/json_utils.c \
	$(SRC_PATH)/http/response.c

UNIT_TEST_UTILS_CFLAGS := -I$(SRC_PATH)/utils/unittests/mock_functions