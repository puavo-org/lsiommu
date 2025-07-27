/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#define _GNU_SOURCE
#include <errno.h>
#include <libgen.h>
#include <libudev.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iommu.h"
#include "pci.h"
#include "string-buffer.h"

static int iommu_group_id(struct udev_device *dev)
{
	const char *sysfs_path;
	STRING_BUFFER(path_buf, 512);
	char target[512];
	char *endptr;
	ssize_t len;
	long id;
	int ret = 0;

	sysfs_path = udev_device_get_syspath(dev);
	if (!sysfs_path)
		return -ENOENT;

	string_buffer_append(path_buf, sysfs_path);
	string_buffer_append(path_buf, "/iommu_group");

	if (path_buf->status & STRING_BUFFER_OVERFLOW) {
		ret = -E2BIG;
		goto out;
	}

	len = readlink((const char *)path_buf->data, target, sizeof(target) - 1);
	if (len < 0) {
		ret = -errno;
		goto out;
	}

	target[len] = '\0';

	errno = 0;
	id = strtol(basename(target), &endptr, 10);
	if (errno == ERANGE || *endptr != '\0') {
		ret = -EINVAL;
		goto out;
	}

	ret = (int)id;
out:
	return ret;
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

static int iommu_get_group(struct udev *udev,
			   struct udev_list_entry *dev_list_entry,
			   struct iommu_group *groups, size_t *groups_cnt,
			   size_t groups_size)
{
	struct udev_device *dev;
	struct pci_device *pci_dev;
	struct iommu_group *target;
	const char *path;
	int group_id;
	size_t i;
	int ret = 0;

	path = udev_list_entry_get_name(dev_list_entry);
	dev = udev_device_new_from_syspath(udev, path);
	if (!dev)
		return 0;

	group_id = iommu_group_id(dev);
	if (group_id < 0) {
		ret = 0;
		goto out;
	}

	target = NULL;
	for (i = 0; i < *groups_cnt; i++) {
		if (groups[i].id == group_id) {
			target = &groups[i];
			break;
		}
	}

	if (!target) {
		if (*groups_cnt >= groups_size) {
			ret = -E2BIG;
			goto out;
		}

		target = &groups[*groups_cnt];
		target->id = group_id;
		target->device_count = 0;
		(*groups_cnt)++;
	}

	if (target->device_count >= IOMMU_GROUP_NR_DEVICES) {
		ret = -E2BIG;
		goto out;
	}

	pci_dev = &target->devices[target->device_count];
	ret = iommu_read_pci_device(dev, pci_dev);
	if (ret < 0) {
		ret = 0;
		goto out;
	}

	target->device_count++;

out:
	udev_device_unref(dev);
	return ret;
}

ssize_t iommu_groups_read(struct iommu_group *groups, size_t groups_size)
{
	struct udev *udev = NULL;
	struct udev_enumerate *enumerate = NULL;
	struct udev_list_entry *devices;
	struct udev_list_entry *dev_list_entry;
	size_t groups_cnt = 0;
	int ret = 0;

	udev = udev_new();
	if (!udev) {
		ret = -ENOMEM;
		goto out;
	}

	enumerate = udev_enumerate_new(udev);
	if (!enumerate) {
		ret = -ENOMEM;
		goto out_unref_udev;
	}

	udev_enumerate_add_match_subsystem(enumerate, "pci");
	udev_enumerate_scan_devices(enumerate);

	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices)
	{
		ret = iommu_get_group(udev, dev_list_entry, groups, &groups_cnt,
				      groups_size);
		if (ret != 0)
			goto out_unref_enumerate;
	}

	ret = iommu_groups_sort(groups, groups_cnt);
	if (ret != 0)
		goto out_unref_enumerate;

	ret = groups_cnt;

out_unref_enumerate:
	udev_enumerate_unref(enumerate);
out_unref_udev:
	udev_unref(udev);
out:
	return ret;
}
