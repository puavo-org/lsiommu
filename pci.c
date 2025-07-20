/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "pci.h"
#include "strbuf.h"

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
	int i, ret;

	for (i = 0; i < len; i++) {
		ret = parse_hex_digit(src[i], &digit);
		if (ret < 0)
			return ret;

		acc = (acc << 4) | digit;
	}

	*value = acc;
	return 0;
}

int pci_dev_read_addr(const char *sysname, uint32_t *addr)
{
	unsigned int domain = 0, bus, slot, func;
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

void pci_dev_read_prop_string(struct pci_device *pci_dev, struct strbuf *out)
{
	struct pci_dev_props *props = &pci_dev->props;

	strbuf_append(out, props->bdf);
	strbuf_append(out, " Class ");
	strbuf_append(out, props->class + 2);
	strbuf_append(out, ": Vendor ");
	strbuf_append(out, props->vendor + 2);
	strbuf_append(out, " Device ");
	strbuf_append(out, props->device + 2);
	strbuf_append(out, " [");
	strbuf_append(out, props->vendor + 2);
	strbuf_append(out, ":");
	strbuf_append(out, props->device + 2);
	strbuf_append(out, "]");

	if (props->has_revision) {
		strbuf_append(out, " (rev ");
		strbuf_append(out, props->revision + 2);
		strbuf_append(out, ")");
	}
}
