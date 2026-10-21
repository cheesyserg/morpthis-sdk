# morpthis-sdk/app.mk
#
# Include this at the end of an app's Makefile:
#
#     APP_ID   := calculator
#     APP_SRCS := src/main.c
#     include $(MORPTHIS_SDK)/app.mk
#
# Inputs
#   APP_ID          required; lowercase, no spaces. Also the binary name.
#   APP_SRCS        required; .c and .cpp sources, relative to the app folder.
#   APP_KIND        plugin (default, the in-process UI model) or exe.
#   APP_INCLUDES    optional -I flags.
#   APP_ASSETS      optional files copied into the package (fonts, data).
#   APP_LIBS        optional libraries.
#   APP_CFLAGS      optional extra compiler flags.
#   APP_LDFLAGS     optional extra linker flags.
#   APP_TEST_SCRIPT semicolon-separated host test script; default "quit".
#   APP_TEST_ARGS   arguments for a host exe.
#
# Environment
#   MORPTHIS_SDK      path to the SDK; defaults to this file's directory.
#   MORPTHIS_MODE     device (default) or host.
#   CROSS_CC/CROSS_CXX
#                     cross compilers; default mipsel-rockbox-linux-gnu-*.
#   CROSS_READELF     readelf used by check-abi.
#
# Targets: all, stage, check-abi, test, clean.
#
# Output:
#   build/<mode>/<APP_ID>          the plugin or executable
#   build/<mode>/app/<APP_ID>/     stage: binary, app.json, icon.png
#
# On the device a plugin is a shared object with no extension, named after the
# app, and `entry` in app.json names it. It leaves the SDK symbols undefined;
# the player exports them with -rdynamic and the dynamic linker resolves them
# when the player dlopen()s the plugin.

MORPTHIS_SDK ?= $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
MORPTHIS_MODE ?= device
APP_KIND ?= plugin
APP_TEST_SCRIPT ?= quit
APP_TEST_ARGS ?=

ifeq ($(strip $(APP_ID)),)
$(error APP_ID is required)
endif
ifeq ($(strip $(APP_SRCS)),)
$(error APP_SRCS is required)
endif
ifeq ($(filter plugin exe,$(APP_KIND)),)
$(error APP_KIND must be plugin or exe, not "$(APP_KIND)")
endif
ifneq ($(filter device host,$(MORPTHIS_MODE)),$(MORPTHIS_MODE))
$(error MORPTHIS_MODE must be device or host, not "$(MORPTHIS_MODE)")
endif

BUILD_DIR := build/$(MORPTHIS_MODE)
OBJ_DIR := $(BUILD_DIR)/obj
APP_BIN := $(BUILD_DIR)/$(APP_ID)
STAGE_DIR ?= $(BUILD_DIR)/app/$(APP_ID)

APP_CXX_SRCS := $(filter %.cpp,$(APP_SRCS))
APP_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(APP_SRCS))
APP_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(APP_OBJS))

# ---------------------------------------------------------------------------
# Flags
# ---------------------------------------------------------------------------

HOST_CC ?= cc
HOST_CXX ?= c++
CROSS_CC ?= mipsel-rockbox-linux-gnu-gcc
CROSS_CXX ?= mipsel-rockbox-linux-gnu-g++
CROSS_READELF ?= mipsel-rockbox-linux-gnu-readelf

ifeq ($(MORPTHIS_MODE),host)
MORPTHIS_CC := $(HOST_CC)
MORPTHIS_CXX := $(HOST_CXX)
MORPTHIS_CFLAGS := -O2 -g -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L \
	-I$(MORPTHIS_SDK)/include -I. $(APP_INCLUDES) $(APP_CFLAGS)
MORPTHIS_CXXFLAGS := -O2 -g -Wall -Wextra -std=c++17 \
	-I$(MORPTHIS_SDK)/include -I. $(APP_INCLUDES) $(APP_CFLAGS)
MORPTHIS_HOST_LIB := $(MORPTHIS_SDK)/build/host/libmorpthis_host.a
ifeq ($(APP_KIND),plugin)
MORPTHIS_LDFLAGS := -pthread -lm $(APP_LDFLAGS)
else
MORPTHIS_LDFLAGS := $(APP_LDFLAGS)
endif
else
MORPTHIS_CC := $(CROSS_CC)
MORPTHIS_CXX := $(CROSS_CXX)
MORPTHIS_CFLAGS := -O2 -g -Wall -Wextra -std=c11 -D_FILE_OFFSET_BITS=64 \
	-march=mips32r2 -mno-madd4 -fvisibility=hidden \
	-I$(MORPTHIS_SDK)/include -I. $(APP_INCLUDES) $(APP_CFLAGS)
MORPTHIS_CXXFLAGS := -O2 -g -Wall -Wextra -std=c++17 -D_FILE_OFFSET_BITS=64 \
	-march=mips32r2 -mno-madd4 -fvisibility=hidden \
	-I$(MORPTHIS_SDK)/include -I. $(APP_INCLUDES) $(APP_CFLAGS)
ifeq ($(APP_KIND),plugin)
MORPTHIS_CFLAGS += -fPIC
MORPTHIS_CXXFLAGS += -fPIC
MORPTHIS_LDFLAGS := -shared -static-libgcc $(APP_LDFLAGS)
else
MORPTHIS_LDFLAGS := -static-libgcc $(APP_LDFLAGS)
endif
endif

ifeq ($(strip $(APP_CXX_SRCS)),)
MORPTHIS_LINKER := $(MORPTHIS_CC)
else
MORPTHIS_LINKER := $(MORPTHIS_CXX)
ifeq ($(MORPTHIS_MODE),device)
MORPTHIS_LDFLAGS += -static-libstdc++
endif
endif

ifeq ($(MORPTHIS_MODE),host)
ifeq ($(APP_KIND),plugin)
MORPTHIS_LIBS := $(MORPTHIS_HOST_LIB) $(APP_LIBS)
else
MORPTHIS_LIBS := $(APP_LIBS)
endif
else
MORPTHIS_LIBS := $(APP_LIBS)
endif

ifeq ($(MORPTHIS_MODE),host)
ifeq ($(APP_KIND),plugin)
APP_LINK_DEPS := $(APP_OBJS) $(MORPTHIS_HOST_LIB)
else
APP_LINK_DEPS := $(APP_OBJS)
endif
else
APP_LINK_DEPS := $(APP_OBJS)
endif

# ---------------------------------------------------------------------------
# Rules
# ---------------------------------------------------------------------------

all: $(APP_BIN)

ifeq ($(MORPTHIS_MODE),host)
ifeq ($(APP_KIND),plugin)
$(MORPTHIS_HOST_LIB): $(wildcard $(MORPTHIS_SDK)/src/host/*.c) \
		$(wildcard $(MORPTHIS_SDK)/src/host/*.h)
	$(MAKE) -C $(MORPTHIS_SDK) host
endif
endif

$(APP_BIN): $(APP_LINK_DEPS)
	$(MORPTHIS_LINKER) -o $@ $(APP_OBJS) $(MORPTHIS_LIBS) $(MORPTHIS_LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(MORPTHIS_CC) $(MORPTHIS_CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(MORPTHIS_CXX) $(MORPTHIS_CXXFLAGS) -MMD -MP -c $< -o $@

-include $(APP_OBJS:.o=.d)

stage: $(APP_BIN)
	@rm -rf $(STAGE_DIR)
	@mkdir -p $(STAGE_DIR)
	@cp $(APP_BIN) $(STAGE_DIR)/
	@chmod +x $(STAGE_DIR)/$(APP_ID)
	@if [ -f app.json ]; then cp app.json $(STAGE_DIR)/; fi
	@if [ -f icon.png ]; then cp icon.png $(STAGE_DIR)/; fi
	@for asset in $(APP_ASSETS); do \
		mkdir -p "$(STAGE_DIR)/$$(dirname "$$asset")"; \
		cp -R "$$asset" "$(STAGE_DIR)/$$(dirname "$$asset")/"; \
	done

# The device has glibc 2.22 and GLIBCXX_3.4.21. The toolchain compiles against
# 2.27 and emits newer C++ symbols; -static-libstdc++ keeps those out of the
# file. This catches anything that slips through, exactly as the player's
# check-abi does.
check-abi: $(APP_BIN)
	@if ! command -v $(CROSS_READELF) >/dev/null 2>&1; then \
		echo "check-abi: $(CROSS_READELF) not found, check skipped"; \
		exit 0; \
	fi; \
	bad=$$($(CROSS_READELF) --dyn-syms -W $(APP_BIN) 2>/dev/null \
		| awk '{ for (i = 1; i <= NF; i++) \
			if (match($$i, /@GLIBC_[0-9.]+$$/)) { \
				ver = substr($$i, RSTART + 7); split(ver, v, "."); \
				if (v[1] > 2 || (v[1] == 2 && v[2] > 22)) print "  " $$i; \
			} else if (match($$i, /@GLIBCXX_[0-9.]+$$/)) { \
				ver = substr($$i, RSTART + 9); split(ver, v, "."); \
				if (v[1] > 3 || (v[1] == 3 && (v[2] > 4 || (v[2] == 4 && v[3] > 21)))) print "  " $$i; \
			} }' | sort -u); \
	if [ -n "$$bad" ]; then \
		echo "check-abi: $(APP_BIN) asks for symbols the device does not have:"; \
		echo "$$bad"; \
		exit 1; \
	fi; \
	echo "check-abi: no symbol beyond glibc 2.22 / GLIBCXX_3.4.21."

test: $(APP_BIN)
	@if [ "$(MORPTHIS_MODE)" != "host" ]; then \
		echo "test: build with MORPTHIS_MODE=host"; \
		exit 1; \
	fi; \
	if [ "$(APP_KIND)" = "plugin" ]; then \
		MORPTHIS_APP_FOLDER="$$(pwd)" MORPTHIS_APP_ID="$(APP_ID)" \
		MORPTHIS_HOST_SCRIPT="$(APP_TEST_SCRIPT)" MORPTHIS_RUN_MS=500 \
		$(APP_BIN) $(APP_TEST_ARGS); \
	else \
		$(APP_BIN) $(APP_TEST_ARGS); \
	fi

clean:
	rm -rf build

.PHONY: all stage check-abi test clean
