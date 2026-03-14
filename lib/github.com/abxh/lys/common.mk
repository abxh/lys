.PHONY: all run clean

PROGNAME ?= lys
LYS_TTF ?= 1

SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(SELF_DIR)/common_setup_flags.mk
FRONTEND_DIR := $(SELF_DIR)/frontends/$(LYS_FRONTEND)

ifeq ($(LYS_FRONTEND), sdl)
FONT_DEPS := font_data.h
else
FONT_DEPS :=
endif

CXXFLAGS ?= -std=c++17 -O -Wall -Wextra -pedantic
CXXFLAGS += -DPROGHEADER='"$(PROGNAME)_wrapper.h"'

CFLAGS ?= -std=gnu11 -O -Wall -Wextra -pedantic
CFLAGS += -DPROGHEADER='"$(PROGNAME)_wrapper.h"'

LDFLAGS += -lstdc++

C_SOURCE_FILES := $(FRONTEND_DIR)/liblys.c $(SELF_DIR)/shared.c $(FRONTEND_DIR)/main.c 
C_HEADER_FILES := $(FRONTEND_DIR)/liblys.h $(SELF_DIR)/shared.h $(SELF_DIR)/file_parsers/obj/tiny_obj_loader.h

all: $(PROGNAME)

%_wrapper.fut: $(SELF_DIR)/lys_entry_gen.fut $(PROG_FUT_DEPS)
	cat $< | sed 's/"lys"/"$(PROGNAME)"/' > $@

%.c %.h: %.fut
	futhark $(LYS_BACKEND) --library $<

$(PROGNAME)_wrapper.o: $(PROGNAME)_wrapper.c $(PROGNAME)_wrapper.h
	$(CC) -o $@ -c $< -w -fno-sanitize=all $(CFLAGS)

# font generation snippet taken from commit:
# https://github.com/dpaneda/lys/commit/f075c3e3c43e984a732beeb744faade68d0c8740

font_data.h: $(SELF_DIR)/Inconsolata-Regular.ttf
	python3 -c 'import sys; \
		data = open(sys.argv[1], "rb").read(); \
		print("unsigned char font_data[] = {"); \
		print(",".join(f"0x{b:02x}" for b in data)); \
		print("};")' $< > $@

tiny_obj_loader.o: $(SELF_DIR)/file_parsers/obj/tiny_obj_loader.cpp $(PROGNAME)_wrapper.h
	$(CXX) -I. -I$(SELF_DIR) -c $< -o $@ $(CXXFLAGS)

shared_cpp.o: $(SELF_DIR)/shared.cpp $(PROGNAME)_wrapper.h $(SELF_DIR)/file_parsers/obj/tiny_obj_loader.h
	$(CXX) -I. -I$(SELF_DIR) -c $< -o $@ $(CXXFLAGS)

ifeq ($(shell test futhark.pkg -nt lib; echo $$?),0)
$(PROGNAME):
	futhark pkg sync
	@$(MAKE) # The sync might have resulted in a new Makefile.
else
$(PROGNAME): $(PROGNAME)_wrapper.o shared_cpp.o tiny_obj_loader.o $(FONT_DEPS) $(C_SOURCE_FILES) $(C_HEADER_FILES)
	$(CC) $(PROGNAME)_wrapper.o shared_cpp.o tiny_obj_loader.o \
		$(C_SOURCE_FILES) \
		-I. -I$(SELF_DIR) -o $@ $(CFLAGS) $(LDFLAGS)
endif

run: $(PROGNAME)
	./$(PROGNAME)

clean:
	rm -f $(PROGNAME) $(PROGNAME).c $(PROGNAME).h \
	      $(PROGNAME)_wrapper.* *.o font_data.h
