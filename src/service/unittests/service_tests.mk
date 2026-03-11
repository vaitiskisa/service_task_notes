UNIT_TEST_NOTES_SERVICE_TARGET := $(BUILD_PATH)/notes_service_cmocka_tests

UNIT_TEST_NOTES_SERVICE_SRCS += $(shell find $(SRC_PATH)/service/unittests -type f -name '*.c' -print)
UNIT_TEST_NOTES_SERVICE_SRCS += \
	$(SRC_PATH)/service/notes_service.c \
	$(SRC_PATH)/notes/note.c \
	$(SRC_PATH)/http/response.c \
	$(SRC_PATH)/utils/json_utils.c

UNIT_TEST_NOTES_SERVICE_CFLAGS := -I$(SRC_PATH)/service/unittests/mock_functions