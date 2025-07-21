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
#include "down.h"
#include "iommu.h"
#include "strbuf.h"

static int iommu_group_id(struct udev_device *dev)
{
	const char *sysfs_path;
	char target[512];
	char *endptr;
	ssize_t len;
	long id;

	sysfs_path = udev_device_get_syspath(dev);
	if (!sysfs_path)
		return -ENOENT;

	down(down_strbuf) struct strbuf *path_buf = strbuf_alloc(512);
	if (!path_buf)
		return -ENOMEM;

	strbuf_append(path_buf, sysfs_path);
	strbuf_append(path_buf, "/iommu_group");

	if (path_buf->status & STRBUF_OVERFLOW)
		return -E2BIG;

	len = readlink((const char *)path_buf->data, target, sizeof(target) - 1);
	if (len < 0)
		return -errno;

	target[len] = '\0';

	errno = 0;
	id = strtol(basename(target), &endptr, 10);
	if (errno == ERANGE || *endptr != '\0')
		return -EINVAL;

	return (int)id;
}

static int iommu_read_pci_props(struct udev_device *dev, struct pci_dev_props *props)
{
	const char *revision;
	const char *vendor;
	const char *device;
	const char *class;
	const char *bdf;

	bdf = udev_device_get_sysname(dev);
	if (!bdf)
		return -ENOENT;

	strncpy(props->bdf, bdf, sizeof(props->bdf) - 1);

	vendor = udev_device_get_sysattr_value(dev, "vendor");
	device = udev_device_get_sysattr_value(dev, "device");
	class = udev_device_get_sysattr_value(dev, "class");
	revision = udev_device_get_sysattr_value(dev, "revision");

	if (!vendor || !device || !class)
		return -ENOENT;

	strncpy(props->vendor, vendor, sizeof(props->vendor) - 1);
	strncpy(props->device, device, sizeof(props->device) - 1);
	strncpy(props->class, class, sizeof(props->class) - 1);

	if (revision) {
		props->has_revision = true;
		strncpy(props->revision, revision,
			sizeof(props->revision) - 1);
	} else {
		props->has_revision = false;
	}

	props->valid = true;
	return 0;
}

static int iommu_get_group(struct udev *udev,
			   struct udev_list_entry *dev_list_entry,
			   struct iommu_group *groups, size_t *groups_cnt,
			   size_t groups_size)
{
	struct pci_device *pci_dev;
	struct iommu_group *target;
	const char *path;
	int group_id;
	size_t i;
	int ret;

	path = udev_list_entry_get_name(dev_list_entry);

	down(down_udev_device) struct udev_device *dev =
		udev_device_new_from_syspath(udev, path);
	if (!dev)
		return 0;

	group_id = iommu_group_id(dev);
	if (group_id < 0)
		return 0;

	target = NULL;
	for (i = 0; i < *groups_cnt; i++) {
		if (groups[i].id == group_id) {
			target = &groups[i];
			break;
		}
	}

	if (!target) {
		if (*groups_cnt >= groups_size)
			return -E2BIG;

		target = &groups[*groups_cnt];
		target->id = group_id;
		target->device_count = 0;
		(*groups_cnt)++;
	}

	if (target->device_count >= IOMMU_MAX_DEVICES_PER_GROUP)
		return -E2BIG;

	pci_dev = &target->devices[target->device_count];
	ret = pci_dev_read_addr(udev_device_get_sysname(dev), &pci_dev->addr);
	if (ret)
		return ret;

	if (iommu_read_pci_props(dev, &pci_dev->props) != 0)
		pci_dev->props.valid = false;

	target->device_count++;

	return 0;
}

ssize_t iommu_groups_read(struct iommu_group *groups, size_t groups_size)
{
	struct udev_list_entry *devices;
	struct udev_list_entry *dev_list_entry;
	size_t groups_cnt = 0;
	int ret;

	down(down_udev) struct udev *udev = udev_new();
	if (!udev)
		return -ENOMEM;

	down(down_udev_enumerate)
	struct udev_enumerate *enumerate = udev_enumerate_new(udev);
	if (!enumerate)
		return -ENOMEM;

	udev_enumerate_add_match_subsystem(enumerate, "pci");
	udev_enumerate_scan_devices(enumerate);

	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices) {
		ret = iommu_get_group(udev, dev_list_entry, groups,
				      &groups_cnt, groups_size);
		if (ret != 0)
			return ret;
	}

	ret = iommu_groups_sort(groups, groups_cnt);
	if (ret != 0)
		return ret;

	return groups_cnt;
}
