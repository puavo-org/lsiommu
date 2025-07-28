/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#define _GNU_SOURCE
#include <errno.h>
#include <libgen.h>
#include <libudev.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iommu.h"
#include "pci.h"
#include "string-buffer.h"

static bool iommu_group_id(struct udev_device *dev, unsigned int *id)
{
	STRING_BUFFER(path_buf, 512);
	const char *sysfs_path;
	char target[512];
	long parsed_id;
	char *endptr;
	ssize_t len;

	sysfs_path = udev_device_get_syspath(dev);
	if (!sysfs_path)
		return false;

	string_buffer_append(path_buf, sysfs_path);
	string_buffer_append(path_buf, "/iommu_group");

	if (path_buf->status & STRING_BUFFER_OVERFLOW)
		return false;

	len = readlink((const char *)path_buf->data, target, sizeof(target) - 1);
	if (len < 0)
		return false;

	target[len] = '\0';

	errno = 0;
	parsed_id = strtol(basename(target), &endptr, 10);
	if (errno == ERANGE || *endptr != '\0' || parsed_id < 0)
		return false;

	*id = (unsigned int)parsed_id;
	return true;
}

static int iommu_read_pci_device(struct udev_device *dev,
				 struct pci_device *pci_dev)
{
	const char *revision, *vendor, *device, *class, *sysname;
	int ret;

	pci_dev->valid = false;

	sysname = udev_device_get_sysname(dev);
	if (!sysname)
		return -EINVAL;

	ret = pci_string_to_addr(sysname, &pci_dev->addr);
	if (ret)
		return ret;

	vendor = udev_device_get_sysattr_value(dev, "vendor");
	device = udev_device_get_sysattr_value(dev, "device");
	class = udev_device_get_sysattr_value(dev, "class");
	revision = udev_device_get_sysattr_value(dev, "revision");

	if (!vendor || !device || !class)
		return 0;

	strncpy(pci_dev->vendor, vendor, sizeof(pci_dev->vendor));
	pci_dev->vendor[sizeof(pci_dev->vendor) - 1] = '\0';

	strncpy(pci_dev->device, device, sizeof(pci_dev->device));
	pci_dev->device[sizeof(pci_dev->device) - 1] = '\0';

	strncpy(pci_dev->class, class, sizeof(pci_dev->class));
	pci_dev->class[sizeof(pci_dev->class) - 1] = '\0';

	if (revision) {
		pci_dev->has_revision = true;
		strncpy(pci_dev->revision, revision, sizeof(pci_dev->revision));
		pci_dev->revision[sizeof(pci_dev->revision) - 1] = '\0';
	} else {
		pci_dev->has_revision = false;
	}

	pci_dev->valid = true;
	return 0;
}

static bool iommu_get_group(struct udev *udev,
			    struct udev_list_entry *dev_list_entry,
			    struct iommu_group *groups, unsigned int *groups_cnt,
			    unsigned int groups_size)
{
	struct pci_device *pci_dev;
	struct iommu_group *target;
	struct udev_device *dev;
	unsigned int group_id;
	const char *path;
	unsigned int i;

	path = udev_list_entry_get_name(dev_list_entry);
	dev = udev_device_new_from_syspath(udev, path);
	if (!dev)
		return true;

	if (!iommu_group_id(dev, &group_id)) {
		udev_device_unref(dev);
		return true;
	}

	target = NULL;
	for (i = 0; i < *groups_cnt; i++) {
		if (groups[i].group_id == group_id) {
			target = &groups[i];
			break;
		}
	}

	if (!target) {
		if (*groups_cnt >= groups_size) {
			udev_device_unref(dev);
			return false;
		}

		target = &groups[*groups_cnt];
		target->group_id = group_id;
		target->nr_devices = 0;
		(*groups_cnt)++;
	}

	if (target->nr_devices >= IOMMU_GROUP_NR_DEVICES) {
		udev_device_unref(dev);
		return false;
	}

	pci_dev = &target->devices[target->nr_devices];
	iommu_read_pci_device(dev, pci_dev);

	target->nr_devices++;
	udev_device_unref(dev);

	return true;
}

bool iommu_groups_read(struct iommu_group *groups, unsigned int *cnt,
		       unsigned int capacity)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	bool ret = true;

	*cnt = 0;

	udev = udev_new();
	if (!udev)
		return false;

	enumerate = udev_enumerate_new(udev);
	if (!enumerate) {
		udev_unref(udev);
		return false;
	}

	udev_enumerate_add_match_subsystem(enumerate, "pci");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices)
	{
		if (!iommu_get_group(udev, dev_list_entry, groups, cnt,
				     capacity)) {
			ret = false;
			break;
		}
	}

	if (ret)
		iommu_groups_sort(groups, *cnt);

	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return ret;
}
