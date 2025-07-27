/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pci.h"
#include "string-buffer.h"

static int parse_hex_digit(char src, uint32_t *value)
{
	if (src >= '0' && src <= '9')
		*value = src - '0';
	else if (src >= 'a' && src <= 'f')
		*value = 10 + src - 'a';
	else if (src >= 'A' && src <= 'F')
		*value = 10 + src - 'A';
	else
		return -EINVAL;

	return 0;
}

static int parse_hex(const char *src, int len, uint32_t *value)
{
	uint32_t acc = 0;
	uint32_t digit;
	int i;
	int ret;

	for (i = 0; i < len; i++) {
		ret = parse_hex_digit(src[i], &digit);
		if (ret < 0)
			return ret;

		acc = (acc << 4) | digit;
	}

	*value = acc;
	return 0;
}

int pci_string_to_addr(const char *sysname, uint32_t *addr)
{
	unsigned int domain = 0;
	unsigned int bus = 0;
	unsigned int slot = 0;
	unsigned int func = 0;
	size_t len;

	if (!sysname)
		return -EINVAL;

	len = strnlen(sysname, 13);

	if (len == 12 && sysname[4] == ':' && sysname[7] == ':' &&
	    sysname[10] == '.') {
		if (parse_hex(sysname, 4, &domain) < 0 ||
		    parse_hex(sysname + 5, 2, &bus) < 0 ||
		    parse_hex(sysname + 8, 2, &slot) < 0 ||
		    parse_hex(sysname + 11, 1, &func) < 0)
			return -EINVAL;
	} else if (len == 7 && sysname[2] == ':' && sysname[5] == '.') {
		if (parse_hex(sysname, 2, &bus) < 0 ||
		    parse_hex(sysname + 3, 2, &slot) < 0 ||
		    parse_hex(sysname + 6, 1, &func) < 0)
			return -EINVAL;
	} else {
		return -EINVAL;
	}

	*addr = (domain << 16) | (bus << 8) | (slot << 3) | func;
	return 0;
}

void pci_addr_to_string(uint32_t addr, char *out, size_t size)
{
	unsigned int domain = (addr >> 16) & 0xffff;
	unsigned int bus = (addr >> 8) & 0xff;
	unsigned int slot = (addr >> 3) & 0x1f;
	unsigned int func = addr & 0x7;

	snprintf(out, size, "%04x:%02x:%02x.%d", domain, bus, slot, func);
}

void string_buffer_to_pci(struct string_buffer *out, struct pci_device *props)
{
	char addr_str[32];

	pci_addr_to_string(props->addr, addr_str, sizeof(addr_str));

	string_buffer_append(out, "Address ");
	string_buffer_append(out, addr_str);
	string_buffer_append(out, " Class ");
	string_buffer_append(out, props->class + 2);
	string_buffer_append(out, " ID ");
	string_buffer_append(out, props->vendor + 2);
	string_buffer_append(out, ":");
	string_buffer_append(out, props->device + 2);

	if (props->has_revision) {
		string_buffer_append(out, " Revision ");
		string_buffer_append(out, props->revision + 2);
	}
}
