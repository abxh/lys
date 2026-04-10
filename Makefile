.PHONY: all run clean prepare

PROGNAME := lys
BUILD_DIR := build

export PROGNAME

# CFLAGS ?= -std=gnu11 -Wall -Wextra -pedantic
# CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic

NOWARN_CFLAGS ?= -O
CFLAGS += -O
CXXFLAGS += -O 

# NOWARN_CFLAGS += -I/opt/rocm/include -L/opt/rocm/lib
# CFLAGS += -I/opt/rocm/include -L/opt/rocm/lib
# CXXFLAGS += -I/opt/rocm/include -L/opt/rocm/lib

# CFLAGS += -fsanitize=undefined
# CXXFLAGS += -fsanitize=undefined
# LDFLAGS += -fsanitize=undefined

LYS_BACKEND := opencl

export NOWARN_CFLAGS CFLAGS CXXFLAGS LDFLAGS LYS_BACKEND

all: run

run: build
	$(MAKE) \
		-C $(BUILD_DIR) \
		-f ../lib/github.com/abxh/lys/common.mk

clean:
	$(MAKE) clean \
		-C $(BUILD_DIR) \
		-f ../lib/github.com/abxh/lys/common.mk

build: build/
	@mkdir -p $(BUILD_DIR)
	@for f in *.fut *.bin *.obj; do \
		ln -sf "../$$f" "$(BUILD_DIR)/$$f"; \
	done
	@ln -snf ../lib $(BUILD_DIR)/lib
