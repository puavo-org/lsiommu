/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef SYSFS_H
#define SYSFS_H

#include <sys/types.h>
#include <stddef.h>

ssize_t sysfs_read_file(const char *path, char *buf, size_t size);

#endif
