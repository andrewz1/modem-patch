TARGET := modem-patch
CC := aarch64-linux-android30-clang

ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
CPPDEFS := -D_GNU_SOURCE
CFLAGS := -O3 -Wall -ffunction-sections -fdata-sections
LDFLAGS := -Wl,--strip-all -Wl,--gc-sections

SUBDIR := src
SRCS := $(wildcard *.c $(foreach v, $(SUBDIR), $(v)/*.c))
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
PHONY :=

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.d: %.c Makefile
	$(CC) -MM -MT '$*.o' -MF $@ $(CFLAGS) $(CPPFLAGS) $(CPPDEFS) $<

%.o: %.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CPPDEFS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

distclean: clean
	find $(ROOT_DIR) -name '*.[do]' -exec rm -f {} \;

-include $(DEPS)

.PHONY: $(PHONY)
