/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#define _GNU_SOURCE
#include <errno.h>
#include <libudev.h>
#include <stdint.h>
#include <string.h>
#include "udev.h"

static int parse_hex_digit(char src, uint32_t *value)
{
	uint32_t tmp = 0;

	if (src >= '0' && src <= '9')
		tmp = src - '0' + 0x10000000;

	if (src >= 'a' && src <= 'f')
		tmp = 10 + src - 'a' + 0x10000000;

	if (src >= 'A' && src <= 'F')
		tmp = 10 + src - 'A' + 0x10000000;

	if (tmp & 0x10000000) {
		*value = tmp & ~0x10000000;
		return 0;
	}

	return -EINVAL;
}

static int parse_hex(const char *src, int len, uint32_t *value)
{
	uint32_t tmp[2] = {0};
	int i, ret;

	for (i = 0; i < len; i++) {
		ret = parse_hex_digit(src[i], &tmp[1]);
		if (ret < 0)
			return ret;

		tmp[0] = (tmp[0] << 4) | tmp[1];
	}

	*value = tmp[0];
	return 0;
}

int udev_read_pci_addr(struct udev_device *dev, uint32_t *addr)
{
	unsigned int domain, bus, slot, func;
	const char *src;
	size_t len;

	src = udev_device_get_sysname(dev);
	if (!src)
		return -ENOMEM;

	len = strnlen(src, 13);

	if (len == 12 && src[4] == ':' && src[7] == ':' && src[10] == '.') {
		if (parse_hex(src, 4, &domain) < 0 ||
		    parse_hex(src + 5, 2, &bus) < 0 ||
		    parse_hex(src + 8, 2, &slot) < 0 ||
		    parse_hex(src + 11, 1, &func) < 0)
			return -EINVAL;
	} else if (len == 7 && src[2] == ':' && src[5] == '.') {
		if (parse_hex(src, 2, &bus) < 0 ||
		    parse_hex(src + 3, 2, &slot) < 0 ||
		    parse_hex(src + 6, 1, &func) < 0)
			return -EINVAL;
	} else {
		return -EINVAL;
	}

	*addr = (domain << 16) | (bus << 8) | (slot << 3) | func;
	return 0;
}
