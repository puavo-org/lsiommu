/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iommu.h"
#include "pci.h"
#include "string-buffer.h"
#include "sysfs.h"

static int iommu_read_pci_device(const char *dev_path, struct pci_device *dev)
{
	const char *bdf;
	STRING_BUFFER(buf, PATH_MAX);
	ssize_t ret;

	dev->valid = false;

	bdf = basename((char *)dev_path);

	ret = pci_string_to_addr(bdf, &dev->addr);
	if (ret)
		return ret;

	string_buffer_append(buf, dev_path);
	string_buffer_append(buf, "/vendor");
	ret = sysfs_read_file((const char *)buf->data, dev->vendor,
			      sizeof(dev->vendor));
	if (ret < 0) {
		ret = 0;
		goto out;
	}

	string_buffer_clear(buf);
	string_buffer_append(buf, dev_path);
	string_buffer_append(buf, "/device");
	ret = sysfs_read_file((const char *)buf->data, dev->device,
			      sizeof(dev->device));
	if (ret < 0) {
		ret = 0;
		goto out;
	}

	string_buffer_clear(buf);
	string_buffer_append(buf, dev_path);
	string_buffer_append(buf, "/class");
	ret = sysfs_read_file((const char *)buf->data, dev->class,
			      sizeof(dev->class));
	if (ret < 0) {
		ret = 0;
		goto out;
	}

	string_buffer_clear(buf);
	string_buffer_append(buf, dev_path);
	string_buffer_append(buf, "/revision");
	dev->has_revision =
		sysfs_read_file((const char *)buf->data, dev->revision,
				sizeof(dev->revision)) >= 0;

	dev->valid = true;
	ret = 0;
out:
	return ret;
}

ssize_t iommu_groups_read(struct iommu_group *groups, size_t groups_size)
{
	const char *pci_devices_path = "/sys/bus/pci/devices";
	STRING_BUFFER(buf, PATH_MAX);
	struct dirent *entry;
	char target_path[PATH_MAX];
	DIR *dir = NULL;
	int ret = 0;
	size_t groups_cnt = 0;

	dir = opendir(pci_devices_path);
	if (!dir) {
		ret = -errno;
		goto out;
	}

	while ((entry = readdir(dir)) != NULL) {
		struct pci_device *pci_dev;
		struct iommu_group *target;
		char *endptr;
		ssize_t len;
		long id;
		size_t i;

		if (entry->d_name[0] == '.')
			continue;

		string_buffer_clear(buf);
		string_buffer_append(buf, pci_devices_path);
		string_buffer_append(buf, "/");
		string_buffer_append(buf, entry->d_name);
		string_buffer_append(buf, "/iommu_group");

		if (buf->status & STRING_BUFFER_OVERFLOW)
			continue;

		len = readlink((const char *)buf->data, target_path,
			       sizeof(target_path) - 1);
		if (len < 0)
			continue;

		target_path[len] = '\0';

		errno = 0;
		id = strtol(basename(target_path), &endptr, 10);
		if (errno != 0 || *endptr != '\0')
			continue;

		target = NULL;
		for (i = 0; i < groups_cnt; i++) {
			if (groups[i].id == id) {
				target = &groups[i];
				break;
			}
		}

		if (!target) {
			if (groups_cnt >= groups_size)
				continue;
			target = &groups[groups_cnt];
			target->id = (int)id;
			target->device_count = 0;
			groups_cnt++;
		}

		if (target->device_count >= IOMMU_GROUP_NR_DEVICES)
			continue;

		pci_dev = &target->devices[target->device_count];

		string_buffer_clear(buf);
		string_buffer_append(buf, pci_devices_path);
		string_buffer_append(buf, "/");
		string_buffer_append(buf, entry->d_name);

		if (buf->status & STRING_BUFFER_OVERFLOW)
			continue;

		if (iommu_read_pci_device((const char *)buf->data, pci_dev) < 0)
			continue;

		target->device_count++;
	}

	iommu_groups_sort(groups, groups_cnt);
	closedir(dir);

out:
	return ret;
}
