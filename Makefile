# morpthis-sdk
#
# make            build the host runtime and the samples
# make host       build build/host/libmorpthis_host.a
# make samples    build every sample for the host
# make test       build and smoke-test every sample on the host
# make device     cross-build every sample (needs the rockbox toolchain)
# make check-abi  cross-build and run the ABI check
# make clean

SDK_ROOT := $(CURDIR)

CC ?= cc
AR ?= ar
CFLAGS ?= -O2 -g
HOST_CFLAGS := $(CFLAGS) -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L \
	-pthread -Iinclude
HOST_SRCS := $(wildcard src/host/*.c)
HOST_OBJS := $(patsubst src/host/%.c,build/host/obj/%.o,$(HOST_SRCS))
HOST_LIB := build/host/libmorpthis_host.a

SAMPLE_DIRS := $(wildcard samples/*/)

.PHONY: all host samples test device check-abi clean

all: host samples

host: $(HOST_LIB)

$(HOST_LIB): $(HOST_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

build/host/obj/%.o: src/host/%.c
	@mkdir -p $(dir $@)
	$(CC) $(HOST_CFLAGS) -MMD -MP -c $< -o $@

-include $(HOST_OBJS:.o=.d)

samples: host
	@set -e; for dir in $(SAMPLE_DIRS); do \
		echo "==> $$dir"; \
		$(MAKE) -C $$dir MORPTHIS_MODE=host MORPTHIS_SDK=$(SDK_ROOT); \
	done

test: samples
	@set -e; for dir in $(SAMPLE_DIRS); do \
		echo "==> $$dir (test)"; \
		$(MAKE) -C $$dir MORPTHIS_MODE=host MORPTHIS_SDK=$(SDK_ROOT) test; \
	done

device:
	@set -e; for dir in $(SAMPLE_DIRS); do \
		echo "==> $$dir (device)"; \
		$(MAKE) -C $$dir MORPTHIS_MODE=device MORPTHIS_SDK=$(SDK_ROOT); \
	done

check-abi: device
	@set -e; for dir in $(SAMPLE_DIRS); do \
		$(MAKE) -C $$dir MORPTHIS_MODE=device MORPTHIS_SDK=$(SDK_ROOT) check-abi; \
	done

clean:
	rm -rf build
	@set -e; for dir in $(SAMPLE_DIRS); do \
		$(MAKE) -C $$dir MORPTHIS_SDK=$(SDK_ROOT) clean; \
	done
