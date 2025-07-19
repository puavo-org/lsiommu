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
#include "radix.h"
#include "strbuf.h"
#include "udev.h"

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

static int iommu_devices_read_description(struct udev_device *dev, char *desc,
					 unsigned int size)
{
	char bdf_tmp[32] = {0};
	const char *revision;
	const char *vendor;
	const char *device;
	const char *class;
	const char *bdf;

	if (size > INT_MAX)
		return -EINVAL;

	bdf = udev_device_get_sysname(dev);
	vendor = udev_device_get_sysattr_value(dev, "vendor");
	device = udev_device_get_sysattr_value(dev, "device");
	class = udev_device_get_sysattr_value(dev, "class");
	revision = udev_device_get_sysattr_value(dev, "revision");

	if (!bdf || !vendor || !device || !class)
		return -ENOENT;

	down(down_strbuf) struct strbuf *buf = strbuf_alloc(512);
	if (!buf)
		return -ENOMEM;

	if (strlen(bdf) > 5)
		strncpy(bdf_tmp, &bdf[5], sizeof(bdf_tmp) - 1);
	else
		strncpy(bdf_tmp, bdf, sizeof(bdf_tmp) - 1);

	strbuf_append(buf, bdf_tmp);
	strbuf_append(buf, " Class ");
	strbuf_append(buf, class + 2);
	strbuf_append(buf, ": Vendor ");
	strbuf_append(buf, vendor + 2);
	strbuf_append(buf, " Device ");
	strbuf_append(buf, device + 2);
	strbuf_append(buf, " [");
	strbuf_append(buf, vendor + 2);
	strbuf_append(buf, ":");
	strbuf_append(buf, device + 2);
	strbuf_append(buf, "]");

	if (revision) {
		strbuf_append(buf, " (rev ");
		strbuf_append(buf, revision + 2);
		strbuf_append(buf, ")");
	}

	if (buf->status & STRBUF_OVERFLOW)
		return -E2BIG;

	strncpy(desc, (char *)buf->data, size - 1);
	desc[size - 1] = '\0';
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
	ret = udev_read_pci_addr(dev, &pci_dev->addr);
	if (ret)
		return ret;

	target->device_count++;

	if (iommu_devices_read_description(dev, pci_dev->description,
					  PCI_DESCRIPTION_SIZE) != 0)
		memcpy(pci_dev->description, PCI_NO_DESCRIPTION,
		       sizeof(PCI_NO_DESCRIPTION));

	return 0;
}

ssize_t iommu_groups_read(struct iommu_group *groups, size_t groups_size)
{
	struct udev_list_entry *devices;
	struct udev_list_entry *dev_list_entry;
	size_t groups_cnt = 0;
	size_t i;
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

	for (i = 0; i < groups_cnt; i++) {
		if (groups[i].device_count > 1) {
			down(down_malloc) struct pci_device *scratch = malloc(
				groups[i].device_count * sizeof(*scratch));
			if (!scratch)
				return -ENOMEM;

			/* Sort PCI devices. */
			radix_sort(groups[i].devices, scratch,
				   groups[i].device_count,
				   sizeof(struct pci_device),
				   offsetof(struct pci_device, addr));
		}
	}

	if (groups_cnt > 1) {
		down(down_malloc) struct iommu_group *scratch =
			malloc(groups_cnt * sizeof(struct iommu_group));
		if (!scratch)
			return -ENOMEM;

		/* Sort IOMMU groups. */
		radix_sort(groups, scratch, groups_cnt,
			   sizeof(struct iommu_group),
			   offsetof(struct iommu_group, id));
	}

	return groups_cnt;
}
