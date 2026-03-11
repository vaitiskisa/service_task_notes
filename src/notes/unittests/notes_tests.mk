UNIT_TEST_NOTES_TARGET := $(BUILD_PATH)/notes_cmocka_tests

UNIT_TEST_NOTES_SRCS += $(shell find $(SRC_PATH)/notes/unittests -type f -name '*.c' -print)
UNIT_TEST_NOTES_SRCS += \
	$(SRC_PATH)/notes/note.c \
	$(SRC_PATH)/http/response.c \
	$(SRC_PATH)/utils/json_utils.c

UNIT_TEST_NOTES_CFLAGS := -I$(SRC_PATH)/notes/unittests/mock_functions