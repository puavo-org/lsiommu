/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef DOWN_H
#define DOWN_H

#include <dirent.h>
#include <stdlib.h>

struct strbuf;

#define down(func) __attribute__((cleanup(func)))

void down_malloc(void *p);
void down_strbuf(struct strbuf **buf);
void down_dir(DIR **dir);

#ifdef CONFIG_LIBUDEV
#include <libudev.h>
void down_udev(struct udev **udev);
void down_udev_device(struct udev_device **dev);
void down_udev_enumerate(struct udev_enumerate **enumerate);
#endif /* CONFIG_LIBUDEV */

#endif /* DOWN_H */
