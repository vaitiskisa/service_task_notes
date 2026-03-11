UNIT_TEST_NOTES_HANDLER_TARGET := $(BUILD_PATH)/notes_handler_cmocka_tests

UNIT_TEST_NOTES_HANDLER_SRCS += $(shell find $(SRC_PATH)/notes_handler/unittests -type f -name '*.c' -print)
UNIT_TEST_NOTES_HANDLER_SRCS += \
	$(SRC_PATH)/notes_handler/notes_handler.c \
	$(SRC_PATH)/notes/note.c \
	$(SRC_PATH)/http/response.c \
	$(SRC_PATH)/utils/json_utils.c

UNIT_TEST_NOTES_HANDLER_CFLAGS := -I$(SRC_PATH)/notes_handler/unittests/mock_functions