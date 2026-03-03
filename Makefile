.PHONY: all run clean prepare

PROGNAME := lys
BUILD_DIR := build

CFLAGS ?= -std=gnu11 -O -Wall -Wextra -pedantic -fsanitize=undefined
CXXFLAGS ?= -std=c++17 -O -Wall -Wextra -pedantic -fsanitize=undefined
LDFLAGS ?= -lstdc++ -fsanitize=undefined

export CFLAGS CXXFLAGS LDFLAGS PROGNAME

all: run

run: build/
	$(MAKE) \
		-C $(BUILD_DIR) \
		-f ../lib/github.com/abxh/lys/common.mk

clean:
	$(MAKE) clean \
		-C $(BUILD_DIR) \
		-f ../lib/github.com/abxh/lys/common.mk

build/:
	@mkdir -p $(BUILD_DIR)
	@for f in *.fut; do \
		ln -sf ../$$f $(BUILD_DIR)/$$f; \
	done
	@ln -snf ../lib $(BUILD_DIR)/lib
