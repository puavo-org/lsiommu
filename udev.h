/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef UDEV_H
#define UDEV_H

#include <stdint.h>
#include <libudev.h>

int udev_read_pci_addr(struct udev_device *dev, uint32_t *addr);

#endif /* UDEV_H */
