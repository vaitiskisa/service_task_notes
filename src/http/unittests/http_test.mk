UNIT_TEST_HTTP_TARGET := $(BUILD_PATH)/http_cmocka_tests

UNIT_TEST_HTTP_SRCS += $(shell find $(SRC_PATH)/http/unittests -type f -name '*.c' -print)
UNIT_TEST_HTTP_SRCS += \
	$(SRC_PATH)/http/router.c \
	$(SRC_PATH)/http/response.c \
	$(SRC_PATH)/http/server.c \
	$(SRC_PATH)/utils/json_utils.c

UNIT_TEST_HTTP_CFLAGS := -I$(SRC_PATH)/http/unittests/mock_functions
