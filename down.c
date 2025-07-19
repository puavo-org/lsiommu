/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <libudev.h>
#include <stdlib.h>

#include "strbuf.h"
#include "down.h"

void down_malloc(void *ptr)
{
	if (ptr && *(void **)ptr)
		free(*(void **)ptr);
}

void down_strbuf(struct strbuf **buf)
{
	if (buf && *buf)
		free(*buf);
}

void down_udev(struct udev **udev)
{
	if (udev && *udev)
		udev_unref(*udev);
}

void down_udev_device(struct udev_device **dev)
{
	if (dev && *dev)
		udev_device_unref(*dev);
}

void down_udev_enumerate(struct udev_enumerate **enumerate)
{
	if (enumerate && *enumerate)
		udev_enumerate_unref(*enumerate);
}
