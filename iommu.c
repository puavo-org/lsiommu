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

static void iommu_devices_sift_down(struct pci_device *devices, size_t start,
				   size_t end)
{
	struct pci_device temp;
	size_t root = start;
	size_t child, swap;
	int ret;

	while ((root * 2) + 1 <= end) {
		child = (root * 2) + 1;
		swap = root;

		if (devices[swap].addr < devices[child].addr)
			swap = child;

		if (child + 1 <= end) {
			if (devices[swap].addr < devices[child + 1].addr)
				swap = child + 1;
		}

		if (swap == root)
			return;

		temp = devices[root];
		devices[root] = devices[swap];
		devices[swap] = temp;

		root = swap;
	}
}

static void iommu_devices_sort(struct pci_device *devices, size_t count)
{
	struct pci_device temp;
	size_t start, end;

	if (count < 2)
		return;

	start = (count / 2) - 1;
	while (start < count) {
		iommu_devices_sift_down(devices, start, count - 1);
		if (start == 0)
			break;
		start--;
	}

	end = count - 1;
	while (end > 0) {
		temp = devices[end];
		devices[end] = devices[0];
		devices[0] = temp;

		end--;
		iommu_devices_sift_down(devices, 0, end);
	}
}

static void iommu_group_sort(struct iommu_group *data,
			     struct iommu_group *scratch, size_t nelem)
{
	unsigned int value;
	size_t cnt[256];
	size_t idx[256];
	size_t b, i, j;
	void *tmp;

	for (b = 0; b < sizeof(int); b++) {
		memset(cnt, 0, sizeof(cnt));

		for (i = 0; i < nelem; i++) {
			value = (unsigned int)data[i].id;
			cnt[(value >> (b * 8)) & 0xFF]++;
		}

		idx[0] = 0;
		for (i = 1; i < 256; i++)
			idx[i] = idx[i - 1] + cnt[i - 1];

		for (i = 0; i < nelem; i++) {
			value = (unsigned int)data[i].id;
			j = idx[(value >> (b * 8)) & 0xFF]++;
			memcpy(&scratch[j], &data[i],
			       sizeof(struct iommu_group));
		}

		tmp = data;
		data = scratch;
		scratch = tmp;
	}
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

	for (i = 0; i < groups_cnt; i++)
		iommu_devices_sort(groups[i].devices, groups[i].device_count);

	if (groups_cnt > 1) {
		down(down_malloc) struct iommu_group *scratch =
			malloc(groups_cnt * sizeof(struct iommu_group));

		if (!scratch) {
			free(groups);
			return -ENOMEM;
		}

		iommu_group_sort(groups, scratch, groups_cnt);
	}

	return groups_cnt;
}
