CSRCS += $(shell find $(SRC_PATH)/http -type f -name '*.c' ! -path '*/unittests/*' -print)
