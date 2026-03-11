UNIT_TEST_NOTES_REPO_TARGET := $(BUILD_PATH)/notes_repository_cmocka_tests

UNIT_TEST_NOTES_REPO_SRCS += $(shell find $(SRC_PATH)/notes_repository/unittests -type f -name '*.c' -print)
UNIT_TEST_NOTES_REPO_SRCS += \
	$(SRC_PATH)/notes_repository/notes_repository.c \
	$(SRC_PATH)/notes/note.c \
	$(SRC_PATH)/http/response.c \
	$(SRC_PATH)/utils/json_utils.c

UNIT_TEST_NOTES_REPO_CFLAGS := -I$(SRC_PATH)/notes_repository/unittests/mock_functions