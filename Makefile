# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright(c) Opinsys Oy 2025

TARGET := lsiommu
DISCOVERY ?= udev

PREFIX ?= /usr/local
MANPREFIX ?= $(PREFIX)/share/man
DESTDIR ?=

CC ?= gcc
CFLAGS := -I. -std=c11 -Wall -Werror -Wpedantic -Wformat=2 -Wno-unused-variable
LDFLAGS ?=
LDLIBS ?=

SOURCES :=

ifeq ($(DISCOVERY), udev)
	SOURCES += iommu/udev.c
	CFLAGS += -DCONFIG_LIBUDEV $(shell pkg-config --cflags libudev)
	LDLIBS += $(shell pkg-config --libs libudev)
else ifeq ($(DISCOVERY), sysfs)
	SOURCES += iommu/sysfs.c
else
	$(error "Invalid value for DISCOVERY")
endif

SOURCES += \
	heap-sort.c \
	main.c \
	pci.c \
	string-buffer.c \
	sysfs.c \
	iommu/json.c \
	iommu/sort.c

CFLAGS += -DCONFIG_DISCOVERY='"$(DISCOVERY)"'
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(MANPREFIX)/man1
	install -m 644 lsiommu.1 $(DESTDIR)$(MANPREFIX)/man1

clean:
	rm -f $(TARGET) $(OBJECTS)
