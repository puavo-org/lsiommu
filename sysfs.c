/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "sysfs.h"

ssize_t sysfs_read_file(const char *path, char *buf, size_t size)
{
	int fd = open(path, O_RDONLY);
	ssize_t len;

	if (fd < 0)
		return -errno;

	len = read(fd, buf, size - 1);
	close(fd);

	if (len < 0)
		return -errno;

	buf[len] = '\0';

	if (len > 0 && buf[len - 1] == '\n') {
		buf[len - 1] = '\0';
		len--;
	}

	return len;
}
